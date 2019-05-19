#include "HTTPServer.h"
#include "HTTPSServer.h"


int main() {
  HTTPSServer server;
  server.makeNonBlocking();
  server.listenOnAddress(8080, "127.0.0.1");
  server.serve();
}