#include "HTTPServer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>

HTTPServer::HTTPServer() : sockfd(socket(AF_INET, SOCK_STREAM, 0)) {
  if(sockfd == -1) {
    perror("socket");
    exit(1);
  }

  int option = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
}

HTTPServer::~HTTPServer() {
	close(sockfd);

  for(auto& fd: fds)
    close(fd.fd);
}

void HTTPServer::makeNonBlocking() {
  int flags = fcntl(sockfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, flags);
}

void HTTPServer::listenOnAddress(int port, const char* ip) {
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(ip);  
  unsigned int size = sizeof(sin);
  if(bind(sockfd, (sockaddr*) &sin, size) == -1) {
    perror("bind");
    exit(1);
  }
  if(listen(sockfd, 1) == -1) {
    perror("listen");
    exit(1);
  }
}

void HTTPServer::serve() {
  fds.push_back({sockfd, POLLIN, 0});

  while(true)
    handleEvents();
}

void HTTPServer::handleEvents() {
  if(poll(fds.data(), fds.size(), -1) == -1) {
    perror("poll");
    return;
  }

  for(unsigned i = 0; i < fds.size(); ++i)
    if(fds[i].revents & POLLIN) {
      if(fds[i].fd == sockfd)
        startNewConnection();
      else {
        currentFdIndex = i;
        if(receiveMessage())
          reactToMessage();
      }
    }
}

void HTTPServer::startNewConnection() {
  sockaddr_in client;
  memset(&client, 0, sizeof(client));
  unsigned int client_len = sizeof(client);

  int receiver = accept(sockfd, (sockaddr*) &client, &client_len);
  if(receiver == -1)
    perror("accept");
  else
    fds.push_back({receiver, POLLIN | POLLOUT, 0});
}

bool HTTPServer::receiveMessage() {
  buffer.resize(100);

  int received = recv(fds[currentFdIndex].fd, const_cast<char*>(buffer.data()), buffer.length(), 0);
  if(received == -1) {
    perror("recv");
    return false;
  }

  if(received == 0) {
    closeConnection();
    return false;
  }

  buffer.resize(received);
  return true;
}

void HTTPServer::reactToMessage() {
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
    if(method == "CONNECT");

    else
      sendResponse();

  }
}

void HTTPServer::endIfNotHTTPRequest() {
  if(buffer.find("HTTP/") == std::string::npos) {
    std::string answer = "501 Not Implemented";
    if(send(fds[currentFdIndex].fd, answer.data(), answer.length(), MSG_NOSIGNAL) == -1)
      perror("send");
    std::cout << "// sent: " << filePath << std::endl << std::endl;

    closeConnection();
    query = "";  
  }
}

void HTTPServer::setMethodInfo() {
  method = buffer.substr(0, buffer.find(" "));
}

void HTTPServer::setContentInfo() {
  if(contentLength == 0) { // contentLength variable not set yet
    std::string lengthStr = query.substr(query.find("Content-Length")+16);
    lengthStr = lengthStr.substr(0, lengthStr.find("\r\n"));
    contentLength = std::atoi(lengthStr.c_str());
  }
  int contentReceived = query.length()-query.find("\r\n\r\n")-4;
  contentLeft = contentLength-contentReceived;
}

void HTTPServer::printInfo() {
  std::cout << query.substr(0, query.find("\r\n\r\n")) << std::endl;
  if(method == "POST") {
    std::string content = query.substr(query.find("\r\n\r\n")+4);
    std::cout << "// received: " << content << std::endl;
  }
}

void HTTPServer::sendResponse() {
  std::string filePath = getFilePath();
  std::string answer = formAnswer(filePath);
  if(send(fds[currentFdIndex].fd, answer.data(), answer.length(), MSG_NOSIGNAL) == -1)
    perror("send");
  std::cout << "// sent: " << filePath << std::endl << std::endl;

  closeConnection();
  query = "";  
}

std::string HTTPServer::getFilePath() {
  std::string filePath = "Strona";
  int begin = method.length()+1;
  int length = query.find("HTTP/") - begin-1;
  filePath += query.substr(begin, length);
  if(filePath == "Strona/")
    filePath += "index.html";
  return filePath;
}

std::string HTTPServer::formAnswer(std::string filePath) {
  std::string answer = "HTTP/1.0 ";

  try {
    std::vector<char> file = readFile(filePath.c_str());
    answer += "200 OK\r\n";
    answer += "Content-Type: ";

    std::string type = filePath.substr(filePath.find(".")+1);
    if(type == "html")
      answer += "text/html; charset=UTF-8";
    else if(type == "jpg")
      answer += "image/jpeg";
    else if(type == "js")
      answer += "application/javascript";
    else
      answer += "application/octet-stream";

    answer += "\r\n";
    answer += "Content-Length: " + std::to_string(file.size()) + "\r\n\r\n";
    for(char l: file)
      answer += l;
    answer += "\r\n\r\n";
  }
  catch(std::ios_base::failure) {
    answer += "404\r\n\r\n";
  }

  return answer;
}

void HTTPServer::closeConnection() {
  close(fds[currentFdIndex].fd);
  fds.erase(fds.begin()+currentFdIndex);
  query = "";
  contentLength = 0;
  contentLeft = 100;
}

// function throws an exception ios_base::failure if file does not exist (or something goes wrong)
std::vector<char> HTTPServer::readFile (const char* path) {
  using namespace std;

  vector<char> result;
  ifstream file (path, ios::in|ios::binary|ios::ate);
  file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  result.resize(file.tellg(), 0);
  file.seekg (0, ios::beg);
  file.read (result.data(), result.size());
  file.close();

  return result;
}