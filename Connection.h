#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "HTTPServer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>

class Connection {
public:
	Connection(int s) : clientSocket(s)
	{
	};

	startConnectionWithServer() {
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
	}

private:
	int serverSocket;
	int clientSocket;

	bool fromClient = true;

	std::string buffer;
	std::string query = "";
	std::string method = "";
	int contentLength = 0;
	int contentLeft = 100;

	long long lastTimestamp;
};

#endif