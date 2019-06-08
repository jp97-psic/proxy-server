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

    void handlePollout(int socket);
    
    // returns -1 or server socket if new connection
    int handlePollin(int socket);

    int getClientSocket() const { return clientSocket; }
    int getServerSocket() const { return serverSocket; }
    int getIncomingSocket() const { return fromClient ? clientSocket : serverSocket; }
    int getOutcomingSocket() const { return fromClient ? serverSocket : clientSocket; }
    bool isEnded() const { return end; }
    bool isTimeExceeded();

private:
    void resetData();

	bool receiveRequest();
	void printInfo();

    void handleOutcoming(int socket);
    int handleIncoming(int socket);

    int handleHTTPRequest();
    void handleHTTPSRequest();

	bool endIfDifferentProtocol();
	void setMethodInfo();
	void setContentInfo();
	bool endOfRequest() { return (method != "POST" && message.find("\r\n\r\n") != std::string::npos) || (method == "POST" && dataProcessed == dataToProcess); }
	bool endOfPostHeader() { return method == "POST" && message.find("\r\n\r\n") != std::string::npos; }

    void handleConnect();
    bool connectWithServer();
	void beginCommunicationWithServer();
	void setDataFromMessage();
    void sendRequest();

    void handleHTTPResponse();
	void handleHTTPSResponse();

	void sendResponse();
    
    int serverSocket = -1;
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

