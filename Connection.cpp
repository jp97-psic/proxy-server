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

  // return elapsed_seconds.count() > timeLimit;
  return false;
}

Connection::Connection(int socket) : clientSocket(socket) {
  lastTimestamp = std::chrono::system_clock::now();
}

void Connection::handleOutcoming() {
  if(sending && !fromClient) {
    sendResponse();
  }
}

void Connection::handleIncoming() {
  if(!sending && fromClient && receiveRequest()) {
    // lastTimestamp = std::chrono::system_clock::now();
    reactToMessage();
  }
}

bool Connection::receiveRequest() {
  buffer.resize(100);

  int received = recv(clientSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
  if(received == -1) {
    perror("recv");
    return false;
  }

  if(received == 0) {
    // end = true;
    return false;
  }

  buffer.resize(received);
  return true;
}

void Connection::reactToMessage() {
  if(message.empty()) { // beginning of communication
    if(endIfNotHTTPRequest())
      return;

    setMethodInfo();
  }

  message += buffer;

  if(endOfHeader()) {
    setContentInfo();
  }

  if(endOfRequest()) {
    printInfo();

    if(buffer.length() > MAX_PAYLOAD) {
      std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
      std::string answer = "HTTP/1.1 413 Payload Too Large \r\n\r\n";
      if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
        perror("send");
      }
      // end = true;
    }

    if(method == "CONNECT") {
      std::cout << "Method CONNECT" << std::endl;
      setDataFromMessage();
      if(connectWithServer()) {
        message = "HTTP/1.1 200 OK \r\n\r\n";
      } else {
        message = "HTTP/1.1 502 Bad Gateway \r\n\r\n";
      }
      dataToProcess = message.length();
      dataProcessed = 0;
      sending = true;
      fromClient = false;
    }
    else {
      beginCommunicationWithServer();
      sendRequest();
      receiveResponse();
    }
  }
}

bool Connection::endIfNotHTTPRequest() {
  if(buffer.find("HTTP/") == std::string::npos) {
    std::string answer = "HTTP/1.1 501 Not Implemented\r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send");
    }

    // end = true;
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
  std::cout << "\nREQUEST HEADER: \n" << message.substr(0, message.find("\r\n\r\n")) << std::endl;
  if(method == "POST") {
    std::string content = message.substr(message.find("\r\n\r\n") + 4);
    std::cout << "\nREQUEST BODY: \n" << content << std::endl;
  }
}

void Connection::beginCommunicationWithServer() {
  setDataFromMessage();
  connectWithServer();

  message = method + " " + filePath + " HTTP/1.0" + message.substr(message.find(" HTTP/")+9);
  dataProcessed = 0;
  sending = true;
}

void Connection::setDataFromMessage() {
  hostname = message.substr(message.find("Host") + 4 + 2);
  hostname = hostname.substr(0, hostname.find("\r\n"));  

  if(method == "CONNECT") {
    isHttps = message.find(":443") != std::string::npos;
    if(isHttps) {
      hostname = hostname.substr(0, hostname.find(":443"));
    }
  } else {
    isHttps = message.find(method + " https://") != std::string::npos;
    int begin = message.find(hostname) + hostname.length();
    int length = message.find("HTTP/") - begin - 1;
    filePath = message.substr(begin, length);
    dataToProcess = message.length();
  }
}

bool Connection::connectWithServer() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(serverSocket == -1) {
    perror("socket");
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
    perror("connect");
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
        perror("send");
      } else {
        sent += status;
      }
    }
  }

  fromClient = false;
  sending = false;
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
      // if(message.empty()) { // beginning of communication
      //   setMethodInfo();
      // }
      message += buffer;
    }
  }

  std::cout << "\nRESPONSE from server:\n" << message << std::endl;

  // close(serverSocket);

  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
}

void Connection::sendResponse() {
  int status = send(clientSocket, message.data() + dataProcessed, message.length() - dataProcessed, MSG_NOSIGNAL);
  if(status == -1) {
    perror("send");
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
