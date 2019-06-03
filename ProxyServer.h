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
	void closeConnection();
	Connection& findConnectionForIncoming();

	void connectToServer(std::string);

	sockaddr_in sin;
	unsigned size;
	const int proxySocket;
	std::vector<pollfd> sockets;
	std::vector<Connection> connections;

	std::string buffer;
	int fdIndex = 0;
	
	std::string query = "";
	std::string method = "";
	int contentLength = 0;
	int contentLeft = 100;
};

#endif /*HTTPSERVER_H*/
