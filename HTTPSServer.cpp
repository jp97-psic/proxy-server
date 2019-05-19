#include "HTTPSServer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>


HTTPSServer::HTTPSServer() {  
  init_openssl();
  ctx = create_context();
  configure_context(ctx);
}

HTTPSServer::~HTTPSServer() {
  SSL_CTX_free(ctx);
  cleanup_openssl();
}

void HTTPSServer::startNewConnection() {
  sockaddr_in client;
  memset(&client, 0, sizeof(client));
  unsigned int client_len = sizeof(client);

  int receiver = accept(sockfd, (sockaddr*) &client, &client_len);
  if(receiver == -1) {
    perror("accept");
  }
  else {
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, receiver);
    acceptSSL();

    fds.push_back({receiver, POLLIN | POLLOUT, 0});
  }
}

bool HTTPServer::receiveMessage() {
  buffer.resize(100);

  acceptSSL();

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

void HTTPSServer::acceptSSL() {
  int ssl_accept_result = SSL_accept(ssl);

  if(ssl_accept_result <= 0) {
    int ssl_accept_err = SSL_get_error(ssl, ssl_accept_result);

    if (ssl_accept_err == SSL_ERROR_WANT_READ) {
      fds[currentFdIndex].events = POLLIN;
    } else if (ssl_accept_err == SSL_ERROR_WANT_WRITE) {
      fds[currentFdIndex].events = POLLOUT;
    } else {
      ERR_print_errors_fp(stderr);
      closeConnection();
    }
  }
}

void HTTPSServer::init_openssl() {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

void HTTPSServer::cleanup_openssl() {
  EVP_cleanup();
}

SSL_CTX * HTTPSServer::create_context() {
  const SSL_METHOD * method;
  SSL_CTX * ctx;

  method = SSLv23_server_method();

  ctx = SSL_CTX_new(method);
  if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void HTTPSServer::configure_context(SSL_CTX * ctx) {
  SSL_CTX_set_ecdh_auto(ctx, 1);

  if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}

