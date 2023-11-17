#ifndef ZC_WS_H
#define ZC_WS_H

#include "ZcConnection.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <queue>
#include <string>

#define MAX_RECV_QUEUE_LENGTH 5

class ZcWs : public ZcConnection
{
public:
  ZcWs(std::string ip, std::queue<std::string> &rcvQueue, bool debug);
  bool connect();
  void disconnect();
  void wait_for_connection();
  void send(std::string message);
  std::string recv(int msgId);
  void loop();
  bool is_connected();

private:
  websockets::WebsocketsClient _ws;
  std::string _pendingRecvMessage;
  volatile bool _recvWaiting;
  int _waitingForMsgId;
  std::queue<std::string> &_rcvQueue;
  bool _debug;
  std::string _ip;
  unsigned long _next_ping_time = 0;
  bool _connected = false;

  void on_message(websockets::WebsocketsMessage message);
  void on_event(websockets::WebsocketsEvent event, String data);
  void on_error(websockets::WebsocketsMessage error);
};

#endif
