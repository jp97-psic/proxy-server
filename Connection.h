#ifndef CONNECTION_H
#define	CONNECTION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include <cstring>

#include <vector>
#include <string>

class Connection {
public:
    Connection(int socket);

    int getIncomingSocket() {
        return fromClient ? clientSocket : serverSocket;
    }

    void handleIncoming();

    bool isEnded() { return end; }

private:
	bool receiveMessage();
	void reactToMessage();
	void printInfo();

	void endIfNotHTTPRequest();
	void setMethodInfo();
	void setContentInfo();
	bool endOfRequest() { return (method != "POST" && query.find("\r\n\r\n") != std::string::npos) || (method == "POST" && contentLeft == 0); }
	bool endOfHeader() { return method == "POST" && query.find("\r\n\r\n") != std::string::npos; }

	void sendResponse();
	std::string getHostname();
	std::string getFilePath(std::string host);
	std::string formAnswer(std::string filePath);
	std::string getAnswer(std::string host, std::string filePath);
    
    int serverSocket;
    int clientSocket;

    bool fromClient = true;

	std::string buffer;
    std::string query = "";
    std::string method = "";
    int contentLength = 0;
    int contentLeft = 100;

    bool end = false;
};

#endif	/* CONNECTION_H */

