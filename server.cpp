#include "ProxyServer.h"

int main() {
  ProxyServer server;
  server.makeNonBlocking();
  server.listenOnAddress(8080, "127.0.0.1");
  server.serve();
}
