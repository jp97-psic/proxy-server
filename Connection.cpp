#include "Connection.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>

#define MAX_PAYLOAD 8 * 1024


bool Connection::isTimeExceeded() {
  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = now-lastTimestamp;

  const double timeLimit = 60.0;

  return elapsed_seconds.count() > timeLimit;
}

Connection::Connection(int socket) : clientSocket(socket) {
  lastTimestamp = std::chrono::system_clock::now();
}

void Connection::handleOutcoming() {
  if(sending && !fromClient) {
    sendResponse();
    lastTimestamp = std::chrono::system_clock::now();
  }
}

void Connection::handleIncoming() {
  if(!sending && fromClient && receiveRequest()) {
    if(isHttps)
      handleHTTPSRequest();
    else
      handleHTTPRequest();
    lastTimestamp = std::chrono::system_clock::now();
  }
}

bool Connection::receiveRequest() {
  buffer.resize(100);

  int received = recv(clientSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
  if(received == -1) {
    perror("recv/receiveRequest");
    return false;
  }

  if(received == 0) {
    return false;
  }

  buffer.resize(received);
  return true;
}

void Connection::handleHTTPRequest() {
  if(message.empty()) {
    if(endIfDifferentProtocol())
      return;
    setMethodInfo();
  }

  message += buffer;

  int endOfHeader = message.find("\r\n\r\n");
  int headerLength = (endOfHeader != std::string::npos) ? endOfHeader : message.length();
  
  if(headerLength > MAX_PAYLOAD) {
    std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
    std::string answer = "HTTP/1.1 413 Payload Too Large \r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send/reactToMessage");
    }
    end = true;
    return;
  }

  if(endOfPostHeader()) {
    setContentInfo();
  }

  if(endOfRequest()) {
    printInfo();

    if(method == "CONNECT") {
      handleConnect();
    }
    else {
      beginCommunicationWithServer();
      sendRequest();
      receiveResponse();
    }
  }
}

void Connection::handleConnect() {
  setDataFromMessage();
  if(connectWithServer()) {
    message = "HTTP/1.1 200 OK \r\n\r\n";
    std::cout << "Connected with server " << serverSocket << std::endl;
  } else {
    message = "HTTP/1.1 502 Bad Gateway \r\n\r\n";
  }

  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
  fromClient = false;
}

void Connection::handleHTTPSRequest() {

}

bool Connection::endIfDifferentProtocol() {
  if(buffer.find("HTTP/") == std::string::npos) {
    std::string answer = "HTTP/1.1 501 Not Implemented\r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send/endIfDifferentProtocol");
    }

    end = true;
    return true;
  }

  return false;
}

void Connection::setMethodInfo() {
  method = buffer.substr(0, buffer.find(" "));
}

void Connection::setContentInfo() {
  if(dataToProcess == 0) { // dataToProcess variable not set yet
    std::string lengthStr = message.substr(message.find("Content-Length") + 16);
    lengthStr = lengthStr.substr(0, lengthStr.find("\r\n"));
    dataToProcess = std::atoi(lengthStr.c_str());
  }

  dataProcessed = message.length() - message.find("\r\n\r\n") - 4;
}

void Connection::printInfo() {
  std::cout << "\nREQUEST HEADER on socket " << clientSocket << ": \n" << message.substr(0, message.find("\r\n\r\n")) << std::endl;
  if(method == "POST") {
    std::string content = message.substr(message.find("\r\n\r\n") + 4);
    std::cout << "\nREQUEST BODY: \n" << content << std::endl;
  }
}

void Connection::beginCommunicationWithServer() {
  setDataFromMessage();
  connectWithServer();

  message = method + " " + filePath + " HTTP/1.0" + message.substr(message.find(" HTTP/")+9);
  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
}

void Connection::setDataFromMessage() {
  hostname = message.substr(message.find("Host") + 4 + 2);

  if(method == "CONNECT") {
    isHttps = true;
    hostname = hostname.substr(0, hostname.find(":443"));
  }
  else {
    hostname = hostname.substr(0, hostname.find("\r\n"));  
    int begin = message.find(hostname) + hostname.length();
    int length = message.find("HTTP/") - begin - 1;
    filePath = message.substr(begin, length);
  }
}

bool Connection::connectWithServer() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(serverSocket == -1) {
    perror("socket/connectWithServer");
    exit(1);
  }

  int option = 1;
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); 

  int flags = fcntl(serverSocket, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(serverSocket, F_SETFL, flags);

  hostent *host = gethostbyname(hostname.data());
  if(host == NULL) {
    printf("%s is unavailable\n", hostname.data());
    exit(1);
  }

  int port = isHttps ? 443 : 80;

  if(isHttps) {
    std::cout << "HTTPS connection" << std::endl;
  } 

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(inet_ntoa(*(in_addr*) (host -> h_addr_list[0])));

  if(connect(serverSocket, (sockaddr*) &sin, sizeof(sin)) == -1 && errno != 115) {
    perror("connect/connectWithServer");
    return false;
  }
  return true;
}

void Connection::sendRequest() {
  std::vector<pollfd> innerFds;
  innerFds.push_back({ serverSocket, POLLOUT, 0 });

  int sent = 0;
  while(sent != message.length()) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLOUT) {
      std::cout << "I am sending this ===> \n" << message << std::endl;
      int status = send(serverSocket, message.data() + sent, message.length() - sent, MSG_NOSIGNAL);
      if(status == -1) {
        perror("send/sendRequest");
      } else {
        sent += status;
      }
    }
  }

  fromClient = false;
  sending = false;
  
  // part for troubleshooting
  // if(message.find("Accept: image/webp") != std::string::npos || message.find("Accept: text/css") != std::string::npos)
  //   std::cout << " :) ";
        
}

void Connection::receiveResponse() {
  std::vector<pollfd> innerFds;
  innerFds.push_back({ serverSocket, POLLIN, 0 });
  
  resetData();
  while(!endOfRequest()) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLIN) {
      buffer.resize(100000);
      int status = recv(serverSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
      buffer.resize(status);
      message += buffer;
    }
  }

  std::cout << "\nRESPONSE from server on socket "<< serverSocket << ":\n";
  if(message.length() > 700)
    std::cout << message.substr(0,400) << "\n(tu ciąg dalszy)\n" << message.substr(message.length()-200);
  else
    std::cout << message;

  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
}

void Connection::sendResponse() {
  int status = send(clientSocket, message.data() + dataProcessed, message.length() - dataProcessed, MSG_NOSIGNAL);
  if(status == -1) {
    perror("send/sendResponse");
  } else {
    dataProcessed += status;
  }
  
  if(dataProcessed == dataToProcess) {
    resetData();
    sending = false;
    fromClient = true;
    
  }

  // end = true;
}

void Connection::resetData() {
  message = "";
  method = "";

  hostname = "";
  filePath = "";

  dataToProcess = 0; // also content size
  dataProcessed = -1; // also content received
}
