#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <cstring>
#include <string>

#include "Connection.h"

class ProxyServer {
public:
	ProxyServer();
	~ProxyServer();

	void makeNonBlocking();
	void listenOnAddress(int port, const char* ip);
	void serve();

private:
	std::vector<char> readFile (const char* path);
	void handleEvents();
	void startNewConnection();
	void closeConnection(int clientSocket);
	std::vector<Connection>::iterator findConnection();

	sockaddr_in sin;
	const int proxySocket;
	std::vector<pollfd> sockets;
	std::vector<Connection> connections;

	unsigned fdIndex = 0;
};

#endif /*HTTPSERVER_H*/
