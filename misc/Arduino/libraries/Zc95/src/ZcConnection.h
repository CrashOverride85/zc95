#ifndef ZC_WSCONNECTION_H
#define ZC_WSCONNECTION_H

#include <string>

class ZcConnection
{
public:
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  virtual void send(std::string message) = 0;
  virtual std::string recv(int msgId) = 0;
  virtual void loop() = 0;
  virtual bool is_connected() = 0;
};

#endif
