#include "ProxyServer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <algorithm>

#define CONNECTION_COUNT_LIMIT 99


ProxyServer::ProxyServer() : proxySocket(socket(AF_INET, SOCK_STREAM, 0)) {
  if(proxySocket == -1) {
    perror("socket");
    exit(1);
  }

  int option = 1;
  setsockopt(proxySocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
}

ProxyServer::~ProxyServer() {
  for(Connection& conn: connections) {
    std::cout << "Closing connection " << conn.getIncomingSocket() << "->" << conn.getOutcomingSocket() << std::endl;

    close(conn.getServerSocket());
    close(conn.getClientSocket());
  }

	close(proxySocket);
}

void ProxyServer::makeNonBlocking() {
  int flags = fcntl(proxySocket, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(proxySocket, F_SETFL, flags);
}

void ProxyServer::listenOnAddress(int port, const char* ip) {
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(ip);  
  unsigned int size = sizeof(sin);

  if(bind(proxySocket, (sockaddr*) &sin, size) == -1) {
    perror("bind");
    exit(1);
  }

  if(listen(proxySocket, 1) == -1) {
    perror("listen");
    exit(1);
  }
}

void ProxyServer::serve() {
  sockets.push_back({ proxySocket, POLLIN, 0 });

  while(true)
    handleEvents();
}

void ProxyServer::handleEvents() {
  if(poll(sockets.data(), sockets.size(), -1) == -1) {
    perror("poll");
    return;
  }

  for(fdIndex = 0; fdIndex < sockets.size(); ++fdIndex) {
    if(sockets[fdIndex].fd == proxySocket) {
      if(sockets[fdIndex].revents & POLLIN) {
        startNewConnection();
      }
    }
    else {
      auto connection = findConnection();

      if(connection == connections.end()) {
        std::cout << "!!! CAN NOT FIND CONNECTION for socket " << sockets[fdIndex].fd << "!!!" << std::endl;
        break;
      }
      
      if(sockets[fdIndex].revents & POLLIN) { 
        int serverSocket = connection->handlePollin(sockets[fdIndex].fd);
        if(serverSocket != -1)
          sockets.push_back({ serverSocket, POLLOUT | POLLIN, 0 });
      }
      else if(sockets[fdIndex].revents & POLLOUT) {
        connection->handlePollout(sockets[fdIndex].fd);
      }
      
      if(connection->isTimeExceeded()) {
        std::cout << "Time exceeded\n";
        closeConnection(connection->getClientSocket());
        break;
      }
      
      if(connection->isEnded()) {
        closeConnection(connection->getClientSocket());
        break;
      }
    }
  }
}

std::vector<Connection>::iterator ProxyServer::findConnection() {
  auto it = find_if(connections.begin(), connections.end(),
    [=](Connection& c) {
        return c.getClientSocket() == sockets[fdIndex].fd ||
        c.getServerSocket() == sockets[fdIndex].fd;
      });

  return it;
}

void ProxyServer::startNewConnection() {
  if(connections.size() > CONNECTION_COUNT_LIMIT) {
    std::cout << "[ERROR] CONNECTIONS LIMIT EXCEDEED" << std::endl << std::endl;
    return;
  }

  sockaddr_in client;
  memset(&client, 0, sizeof(client));
  unsigned int client_len = sizeof(client);

  int receiver = accept(proxySocket, (sockaddr*) &client, &client_len);
  if(receiver == -1) {
    perror("accept");
  }
  else {
    sockets.push_back({ receiver, POLLIN | POLLOUT, 0 });
    connections.emplace_back(receiver);
  }
}

void ProxyServer::closeConnection(int clientSocket) {
  auto connIt = find_if(connections.begin(), connections.end(),
    [=](Connection& c) {return c.getClientSocket() == clientSocket;} );
  int serverSocket = connIt->getServerSocket();
  connections.erase(connIt);

  std::cout << "Closing connection " << clientSocket << "->" << serverSocket << std::endl;

  if(clientSocket > 0) {
    close(clientSocket);

    auto sockIt = find_if(sockets.begin(), sockets.end(),
      [=](pollfd& p) { return p.fd == clientSocket; });
    sockets.erase(sockIt);
  }
  
  if(serverSocket > 0) {
    close(serverSocket);
    auto sockIt = find_if(sockets.begin(), sockets.end(),
      [=](pollfd& p) { return p.fd == serverSocket; });
    sockets.erase(sockIt);
  }
}
