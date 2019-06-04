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
#include <chrono>

class Connection {
public:
    Connection(int socket);

    int getIncomingSocket() { return fromClient ? clientSocket : serverSocket; }
    int getOutcomingSocket() { return fromClient ? serverSocket : clientSocket; }
    void handleIncoming();
    void handleOutcoming();
    bool isEnded() { return end; }
    bool isTimeExceeded();

private:
    void resetData();

	bool receiveRequest();
	void reactToMessage();
	void printInfo();

	bool endIfNotHTTPRequest();
	void setMethodInfo();
	void setContentInfo();
	bool endOfRequest() { return (method != "POST" && message.find("\r\n\r\n") != std::string::npos) || (method == "POST" && dataProcessed == dataToProcess); }
	bool endOfHeader() { return method == "POST" && message.find("\r\n\r\n") != std::string::npos; }

    void connectWithServer();
	void beginCommunicationWithServer();
	void setDataFromMessage();
    void sendRequest();
	void receiveResponse();
	void sendResponse();
    
    int serverSocket;
    int clientSocket;

    bool fromClient = true;
    bool sending = false;
    
	std::chrono::time_point<std::chrono::system_clock> lastTimestamp;

	std::string buffer;
    std::string message = "";
    std::string method = "";
    bool isHttps = false;

    std::string hostname = "";
    std::string filePath = "";

    int dataToProcess = 0; // also content size
    int dataProcessed = -1; // also content received

    bool end = false;
};

#endif	/* CONNECTION_H */

