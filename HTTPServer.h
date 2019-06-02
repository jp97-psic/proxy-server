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

class HTTPServer {
public:
	HTTPServer();
	~HTTPServer();

	void makeNonBlocking();
	void listenOnAddress(int port, const char* ip);
	void serve();

private:
	std::vector<char> readFile (const char* path);
	void handleEvents();
	void startNewConnection();
	void closeConnection();
	void connectToServer(std::string);
	Connection& findConnection(int incomingFd);

	sockaddr_in sin;
	unsigned size;
	const int sockfd;
	std::vector<pollfd> fds;
	std::vector<Connection> connections;

	int currentFdIndex = 0;
};

#endif /*HTTPSERVER_H*/
