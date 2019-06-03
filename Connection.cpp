#include "Connection.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>

Connection::Connection(int socket) : clientSocket(socket) { }

void Connection::handleIncoming() {
  if(receiveMessage()) {
    reactToMessage();
  }
}

bool Connection::receiveMessage() {
  buffer.resize(100);

  int received = recv(clientSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
  if(received == -1) {
    perror("recv");
    return false;
  }

  if(received == 0) {
    end = true;
    return false;
  }

  if(received > 8000) {
    std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
    std::string answer = "HTTP/1.0 413 Payload Too Large \r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send");
    }
    end = true;
    return false;
  }

  buffer.resize(received);
  return true;
}

void Connection::reactToMessage() {
  if(query.empty()) {
    endIfNotHTTPRequest();
    setMethodInfo();
  }

  query += buffer;

  if(endOfHeader()) {
    setContentInfo();
  }

  if(endOfRequest()) {
    printInfo();
    if(method == "CONNECT") {
      std::cout << "Method CONNECT" << std::endl;
    } else {
      sendResponse();
    }
  }
}

void Connection::endIfNotHTTPRequest() {
  if(buffer.find("HTTP/") == std::string::npos) {
    std::string answer = "HTTP/1.0  501 Not Implemented \r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send");
    }
    query = "";  
  }
}

void Connection::setMethodInfo() {
  method = buffer.substr(0, buffer.find(" "));
}

void Connection::setContentInfo() {
  if(contentLength == 0) { // contentLength variable not set yet
    std::string lengthStr = query.substr(query.find("Content-Length") + 16);
    lengthStr = lengthStr.substr(0, lengthStr.find("\r\n"));
    contentLength = std::atoi(lengthStr.c_str());
    std::cout << "Received Content-Length = " << contentLength << std::endl;
  }

  int contentReceived = query.length() - query.find("\r\n\r\n") - 4;
  contentLeft = contentLength - contentReceived;
}

void Connection::printInfo() {
  std::cout << "REQUEST HEADER: \n" << query.substr(0, query.find("\r\n\r\n")) << std::endl;
  if(method == "POST") {
    std::string content = query.substr(query.find("\r\n\r\n") + 4);
    std::cout << "REQUEST BODY: \n" << content << std::endl;
  }
}

void Connection::sendResponse() {
  std::string hostname = getHostname();
  std::string filePath = getFilePath(hostname);
  std::string answer = getAnswer(hostname, filePath);
  
  if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
    perror("send");
  }

  query = "";  
}

std::string Connection::getHostname() {
  std::string hostname = query.substr(query.find("Host") + 4 + 2);
  hostname = hostname.substr(0, hostname.find("\r\n"));
  return hostname;
}

std::string Connection::getFilePath(std::string hostname) {
  int begin = query.find(hostname) + hostname.length();
  int length = query.find("HTTP/") - begin - 1;
  std::string filePath = query.substr(begin, length);
  return filePath;
}

std::string Connection::getAnswer(std::string hostname, std::string filePath) {
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

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  // TODO: add https 443
  sin.sin_port = htons(80);
  sin.sin_addr.s_addr = inet_addr(inet_ntoa(*(in_addr*) (host -> h_addr_list[0])));

  if(connect(serverSocket, (sockaddr*) &sin, sizeof(sin)) == -1 && errno != 115) {
    perror("connect");
    exit(1);
  }

  std::vector<pollfd> innerFds;
  innerFds.push_back({ serverSocket, POLLOUT | POLLIN });

  std::string newQuery = method + " " + filePath + query.substr(query.find(" HTTP/"));
  std::cout << "New query: " << newQuery << std::endl;

  int sent = 0;
  while(sent != newQuery.length()) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLOUT) {
      int status = send(serverSocket, newQuery.data() + sent, newQuery.length() - sent, 0);
      if(status == -1) {
        perror("send");
      } else {
        sent += status;
      }
    }
  }
  
  std::string received = "";
  while(received.find("\r\n\r\n") == std::string::npos) {
    poll(innerFds.data(), innerFds.size(), -1);

    if(innerFds[0].revents & POLLIN) {
      received.resize(100000);
      int status = recv(serverSocket, const_cast<char*>(received.data()), received.length(), 0);
      received.resize(status);
    }
  }
  std::cout << "\nRESPONSE received as client: \n" << received << "\n";
  return received;
}