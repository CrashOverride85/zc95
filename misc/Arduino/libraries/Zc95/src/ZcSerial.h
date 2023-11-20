#ifndef ZC_SERIAL_H
#define ZC_SERIAL_H

#include "ZcConnection.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>
#include <string>

#define MAX_RECV_QUEUE_LENGTH 5

class ZcSerial : public ZcConnection
{
public:
  ZcSerial(HardwareSerial *serial_port, std::queue<std::string> &rcvQueue, bool debug);
  bool connect();
  void disconnect();
  void send(std::string message);
  std::string recv(int msgId);
  void loop();
  bool is_connected();

private:
  void process_message(std::string message);
  void poll();
  void reset_message_buffer();
  
  char _msg_buffer[1024];
  uint16_t _msg_buffer_pos = 0;
  bool _stx_received = false;

  HardwareSerial *_serial_port;
  std::string _pendingRecvMessage;
  volatile bool _recvWaiting;
  int _waitingForMsgId;
  std::queue<std::string> &_rcvQueue;
  bool _debug;
  bool _connected = false;
  unsigned long _reset_sent_time = 0;
};

#endif
