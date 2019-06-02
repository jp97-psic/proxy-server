#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <chrono>

class Connection {
public:
	Connection(int s) : clientSocket(s)	{
		lastOperation = std::chrono::system_clock::now();
	};

	void startConnectionWithServer();
	void handleIncoming();

	int getIncomingSocket() {	return fromClient ? clientSocket : serverSocket; }

private:
	void handleIncomingFromClient();
	void handleIncomingFromServer();
	bool receiveRequest();
	void reactToRequest();
	void endIfNotHTTPRequest();
	void setMethodInfo();
	bool endOfRequest() { return (method != "POST" && data.find("\r\n\r\n") != std::string::npos) || (method == "POST" && dataProcessed == dataToProcess); }
	bool endOfPostHeader() { return method == "POST" && data.find("\r\n\r\n") != std::string::npos; }
	void setContentInfo();
	void printInfo();

	void sendResponse();
	void setHostnameFromRequest();
	void setFilePathFromRequest();

	void beginCommunicationWithServer();
	void sendRequest();
	void receiveResponse();

	bool isTimeExceeded();

	// void currentSocket();

	int serverSocket;
	int clientSocket;

	bool fromClient = true;
	// bool receiving = true;

	std::string buffer;
	int dataToProcess = 0;
	int dataProcessed = 0;

	std::string data = "";
	std::string method = "";

	std::string hostname;
	std::string filePath;

	std::chrono::time_point<std::chrono::system_clock> lastOperation;

	long long lastTimestamp;
};
