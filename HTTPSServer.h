#ifndef HTTPSSERVER_H
#define HTTPSSERVER_H

#include "HTTPServer.h"


class HTTPSServer : public HTTPServer {
public:
  HTTPSServer();
  virtual ~HTTPSServer() override;


protected:
  virtual void startNewConnection() override;
  virtual bool receiveMessage() override;

private:
  void init_openssl();
  void cleanup_openssl();
  SSL_CTX * create_context();
  void configure_context(SSL_CTX * ctx);

  void acceptSSL();

  SSL_CTX * ctx;
  SSL * ssl;
};

#endif
