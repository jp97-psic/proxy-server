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
	close(proxySocket);

  for(auto& fd: sockets) {
    close(fd.fd);
  }
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
      Connection& connection = findConnection();
      
      // if(connection.isTimeExceeded()) {
      //   closeConnection();
      // }
      
      if(sockets[fdIndex].revents & POLLIN) {
        connection.handleIncoming();
      }
      // else if(sockets[fdIndex].revents & POLLOUT) {
      //   connection.handleOutcoming();
      // }

      if(connection.isEnded())
        closeConnection();
    }
  }
}

Connection& ProxyServer::findConnection() {
  auto it = find_if(connections.begin(), connections.end(),
    [=](Connection c) { return c.getIncomingSocket() == sockets[fdIndex].fd; });
  
  if(it == connections.end()) {
    it = find_if(connections.begin(), connections.end(),
    [=](Connection c) { return c.getOutcomingSocket() == sockets[fdIndex].fd; });
  }

  return *it;
}

void ProxyServer::startNewConnection() {
  if(connections.size() > CONNECTION_COUNT_LIMIT) {
    std::cout << "[ERROR] CONNECTIONS LIMIT EXCEDEED" << std::endl << std::endl;
    closeConnection();
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
    sockets.push_back({ receiver, POLLIN, 0 });
    connections.push_back(receiver);
  }
}

void ProxyServer::connectToServer(std::string hostname) {

}

void ProxyServer::closeConnection() {
  close(sockets[fdIndex].fd);

  sockets.erase(sockets.begin() + fdIndex);

  auto it = find_if(connections.begin(), connections.end(),
    [=](Connection c) { return c.getIncomingSocket() == sockets[fdIndex].fd; });
  connections.erase(it);
}

// Informacja o błędzie:

// Brak wczytania pełnego nagłówka oraz błąd z przetworzeniem ciała:

// REQUEST HEADER:
// POST http://ocsp.pki.goog/GTSGIAG3 HTTP/1.1
// Host: ocsp.pki.goog
// User-Agent: Mozilla/5.0 (Windows N

// REQUEST BODY:
// T http://ocsp.pki.goog/GTSGIAG3 HTTP/1.1
// Host: ocsp.pki.goog
// User-Agent: Mozilla/5.0 (Windows N
// New message: POST /GTSGIAG3 HTTP/1.1
// Host: ocsp.pki.goog
// User-Agent: Mozilla/5.0 (Windows N