#include "HTTPServer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <algorithm>

#define DEBUG 1

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

  for(auto& fd: fds) {
    close(fd.fd);
  }
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
  fds.push_back({ sockfd, POLLIN, 0 });

  while(true)
    handleEvents();
}

void HTTPServer::handleEvents() {
  if(poll(fds.data(), fds.size(), 1000) == -1) {
    perror("poll");
    return;
  }

  for(currentFdIndex = 0; currentFdIndex < fds.size(); ++currentFdIndex)
  {
    if(fds[currentFdIndex].fd == sockfd) {
      if(fds[currentFdIndex].revents & POLLIN) {
        std::cout << "started new connection!" << std::endl;
        startNewConnection();
      }
      return;
    }
    
    Connection connection = findConnection();

    if(connection.isTimeExceeded()) {
      closeConnection();
      return;
    }

    if(fds[currentFdIndex].revents & POLLIN) {
      std::cout << "same old connection" << std::endl;
      connection.handleIncoming();
      return;
    }

    if(connection.hasDataToSend() && (fds[currentFdIndex].revents & POLLOUT)) {
      connection.sendData();
      return;
    }
  }
}

Connection& HTTPServer::findConnection() {
  auto iterator = find_if(connections.begin(), connections.end(),
    [=](Connection c) { c.getIncomingSocket == fds[i].fd; });

  // if (iterator == connections.end()) {
  //   std::cerr << "connection couldn't be found!";
  //   return;
  // }

  return *iterator;
}

void HTTPServer::startNewConnection() {
  sockaddr_in client;
  memset(&client, 0, sizeof(client));
  unsigned int client_len = sizeof(client);

  int receiver = accept(sockfd, (sockaddr*) &client, &client_len);
  if(receiver == -1) {
    perror("accept");
  } else {
    if(fds.size() < 102) {
      fds.push_back({ receiver, POLLIN, 0 });
      connections.push_back({ receiver });
    } else {
      // # define limit 100
      // # define system 1 our socket
      // respond wit 500
      std::cout << "[ERROR] CONNECTIONS LIMIT EXCEDEED" << std::endl << std::endl;
    }
  }
}

void HTTPServer::connectToServer(std::string hostname) {

}

void HTTPServer::closeConnection() {
  std::cout << "Connection " << currentFdIndex << " closed GOODBYE!" << std::endl;
  close(fds[currentFdIndex].fd);

  fds.erase(fds.begin() + currentFdIndex);
  connections.erase(findConnection());
  query = "";
  contentLength = 0;
  contentLeft = 100;
}
