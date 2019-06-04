#include "Connection.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>




bool Connection::isTimeExceeded() {
  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = now-lastTimestamp;
  lastTimestamp = now;

  const double timeLimit = 60.0;

  return elapsed_seconds.count() > timeLimit;
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
  if(receiveRequest()) {
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

    // if(buffer.length() > 8096) {
    //   std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
    //   std::string answer = "HTTP/1.1 413 Payload Too Large \r\n\r\n";
    //   if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
    //     perror("send");
    //   }
    //   // end = true;
    //   // return false;
    // }

    if(method == "CONNECT") {
      std::cout << "Method CONNECT" << std::endl;
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
    std::string answer = "HTTP/1.1  501 Not Implemented\r\n\r\n";
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

  message = method + " " + filePath + message.substr(message.find(" HTTP/"));
  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
}

void Connection::setDataFromMessage() {
  hostname = message.substr(message.find("Host") + 4 + 2);
  hostname = hostname.substr(0, hostname.find("\r\n"));
  
  int begin = message.find(hostname) + hostname.length();
  int length = message.find("HTTP/") - begin - 1;
  filePath = message.substr(begin, length);

  ifHttps = message.find(":443") == std::string::npos;
}

void Connection::connectWithServer() {
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

  int port = ifHttps ? 443 : 80;

  if(ifHttps) {
    std::cout << "HTTPS connection" << std::endl;
  } 

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(inet_ntoa(*(in_addr*) (host -> h_addr_list[0])));

  if(connect(serverSocket, (sockaddr*) &sin, sizeof(sin)) == -1 && errno != 115) {
    perror("connect");
    exit(1);
  }
}

void Connection::sendRequest() {
  std::vector<pollfd> innerFds;
  innerFds.push_back({ serverSocket, POLLOUT, 0 });

  int sent = 0;
  while(sent != message.length()) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLOUT) {
      int status = send(serverSocket, message.data() + sent, message.length() - sent, 0);
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
  while(endOfRequest()) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLIN) {
      buffer.resize(100000);
      int status = recv(serverSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
      buffer.resize(status);

      if(message.empty()) { // beginning of communication
        setMethodInfo();
      }
      message += buffer;
    }
  }

  dataToProcess = message.length();
  dataProcessed = 0;
  sending = true;
}

void Connection::sendResponse() {
  if(dataProcessed == dataToProcess) {
    resetData();
    return;
  }

  int status = send(serverSocket, message.data() + dataProcessed, message.length() - dataProcessed, 0);
  if(status == -1) {
    perror("send");
  } else {
    dataProcessed += status;
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