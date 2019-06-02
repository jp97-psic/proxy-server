#include "Connection.h"

bool Connection::isTimeExceeded() {
    auto now = std::chrono::system_clock::now();
 
    std::chrono::duration<double> elapsed_seconds = end-start;

    return elapsed_seconds.count() > 60.0;
}

void Connection::handleIncoming() {
  if(fromClient)
    handleIncomingFromClient();
  else
    handleIncomingFromServer();
}

void Connection::handleIncomingFromClient()	{
  if (receiveRequest()) {
    reactToRequest();
  }
}

// int Connection::currentSocket() {
//   if((fromClient && receiving) || (!fromClient && !receiving))
//     return clientSocket;
//   else
//     return serverSocket;  
// }

bool Connection::receiveRequest() {
  buffer.resize(100);

  int received = recv(clientSocket, const_cast<char*>(buffer.data()), buffer.length(), 0);
  if(received == -1) {
    perror("recv");
    return false;
  }

  if(received == 0) {
    // closeConnection();
    return false;
  }

  buffer.resize(received);
  return true;
}

void Connection::reactToRequest() {
  if(data.empty()) {
    endIfNotHTTPRequest();
    setMethodInfo();
  }

  if(data.length() + buffer.length() > 8096) {
    data = "HTTP/1.0 413 Payload Too Large \r\n\r\n";
    fromClient = false;
  //   // # define 
    // std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
    // std::string answer = "HTTP/1.0 413 Payload Too Large \r\n\r\n";
  //   if(send(fds[currentFdIndex].fd, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
  //     perror("send");
  //   }
  //   closeConnection();
  //   return false;
  }

  data += buffer;

  if(endOfPostHeader()) {
    setContentInfo();
  }

  if(endOfRequest()) {
    printInfo();
    if(method == "CONNECT") {
      startConnectionWithServer();
    } else {
      beginCommunicationWithServer();
    }
  }
}

void Connection::startConnectionWithServer() {
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
    data = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
  }
  else {
    data = "HTTP/1.0 200 OK \r\n\r\n";
  }

  fromClient = false;
}

void Connection::endIfNotHTTPRequest() {
  if(buffer.find("HTTP/") == std::string::npos) {
    std::string answer = "HTTP/1.0  501 Not Implemented \r\n\r\n";
    if(send(clientSocket, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
      perror("send");
    }
    // closeConnection();
    data = "";  
  }
}

void Connection::setMethodInfo() {
  method = buffer.substr(0, buffer.find(" "));
}

void Connection::setContentInfo() {
  if(dataToProcess == 0) { // dataToProcess variable not set yet
    std::string lengthStr = data.substr(data.find("Content-Length") + 16);
    lengthStr = lengthStr.substr(0, lengthStr.find("\r\n"));
    dataToProcess = std::atoi(lengthStr.c_str());
    std::cout << "Received Content-Length = " << dataToProcess << std::endl;
  }

  dataProcessed = data.length() - data.find("\r\n\r\n") - 4;
}

void Connection::printInfo() {
  std::cout << "REQUEST HEADER: \n" << data.substr(0, data.find("\r\n\r\n")) << std::endl;
  if(method == "POST") {
    std::string content = data.substr(data.find("\r\n\r\n") + 4);
    std::cout << "REQUEST BODY: \n" << content << std::endl;
  }
}

void Connection::setHostnameFromRequest() {
  hostname = data.substr(data.find("Host") + 4 + 2);
  hostname = hostname.substr(0, hostname.find("\r\n"));
}

void Connection::setFilePathFromRequest() {
  int begin = data.find(hostname) + hostname.length();
  int length = data.find("HTTP/") - begin - 1;
  filePath = data.substr(begin, length);
}

void Connection::beginCommunicationWithServer() {
  setHostnameFromRequest();
  setFilePathFromRequest();

  // connect with server and add his fd to pollfd
  // fds.push_back({ serverSocket, POLLOUT });

  data = method + " " + filePath + data.substr(data.find(" HTTP/"));
  dataToProcess = data.length();
  dataProcessed = 0;



  // int sent = 0;
  // while(sent != newdata.length()) {
  //   poll(innerFds.data(), innerFds.size(), -1);

  //   if(innerFds[0].revents & POLLOUT) {
  //     int status = send(serverSocket, newdata.data() + sent, newdata.length() - sent, 0);
  //     if(status == -1) {
  //       perror("send");
  //     } else {
  //       sent += status;
  //     }
  //   }
  // }
  
  // std::string received = "";
  // while(received.find("\r\n\r\n") == std::string::npos) {
  //   poll(innerFds.data(), innerFds.size(), -1);

  //   if(innerFds[0].revents & POLLIN) {
  //     received.resize(100000);
  //     int status = recv(serverSocket, const_cast<char*>(received.data()), received.length(), 0);
  //     received.resize(status);
  //   }
  // }
  // std::cout << "\nRESPONSE received as client: \n" << received << "\n";
  // return received;
}

void Connection::sendRequest() {
  if(dataProcessed == dataToProcess) {
    fromClient = false;
    data = "";
  }

  int sent = send(serverSocket, data.data()+dataProcessed, data.length()-dataProcessed, 0);
  if(sent == -1) {
    perror("send");
  } else {
    dataProcessed += sent;
  }
}

void Connection::receiveResponse() {
  
  // std::string received = "";
  // while(received.find("\r\n\r\n") == std::string::npos) {
  //   poll(innerFds.data(), innerFds.size(), -1);

  //   if(innerFds[0].revents & POLLIN) {
  //     received.resize(100000);
  //     int status = recv(serverSocket, const_cast<char*>(received.data()), received.length(), 0);
  //     received.resize(status);
  //   }
  // }
  // std::cout << "\nRESPONSE received as client: \n" << received << "\n";
  // return received;
}

void Connection::sendResponse() {
  if(dataProcessed == dataToProcess) {
    fromClient = true;
    data = "";
  }

  if(send(serverSocket, data.data(), data.length(), MSG_NOSIGNAL) == -1) {
    perror("send");
  } 
}

void Connection::handleIncomingFromServer() {
  if(data.empty()) {
    setMethodInfo();
  }

  data += buffer;

  if(data.size() > 8096) {
  //   // # define 
    // std::cout << "[ERROR] 413 Payload Too Large" << std::endl;
    // std::string answer = "HTTP/1.0 413 Payload Too Large \r\n\r\n";
  //   if(send(fds[currentFdIndex].fd, answer.data(), answer.length(), MSG_NOSIGNAL) == -1) {
  //     perror("send");
  //   }
  //   closeConnection();
  //   return false;
  }

  // check 8096

  if(endOfPostHeader()) {
    setContentInfo();
  }

  if(endOfRequest()) {
    printInfo();
    beginCommunicationWithServer();
  }
}
