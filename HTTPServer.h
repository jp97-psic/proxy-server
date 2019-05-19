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
#include <openssl/ssl.h>
#include <openssl/err.h>


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
	bool receiveMessage();
	void reactToMessage();
	void endIfNotHTTPRequest();
	void setMethodInfo();
	bool endOfRequest() { return (method != "POST" && query.find("\r\n\r\n") != std::string::npos) || (method == "POST" && contentLeft == 0); }
	bool endOfHeader() { return method == "POST" && query.find("\r\n\r\n") != std::string::npos; }
	void setContentInfo();
	void sendResponse();
	std::string getFilePath();
	std::string formAnswer(std::string filePath);
	void printInfo();
	void closeConnection();

	sockaddr_in sin;
	unsigned size;
	const int sockfd;
	std::vector<pollfd> fds;

	std::string buffer;
	std::string query = "";
	std::string method = "";
	int contentLength = 0;
	int contentLeft = 100;
	int currentFdIndex = 0;
};

#endif /*HTTPSERVER_H*/
