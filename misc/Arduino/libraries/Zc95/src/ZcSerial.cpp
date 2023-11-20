#include "ZcSerial.h"

ZcSerial::ZcSerial(HardwareSerial *serial_port, std::queue<std::string> &rcvQueue, bool debug) : _rcvQueue(rcvQueue)
{
  _debug = debug;
  _waitingForMsgId = 0;
  _recvWaiting = false;
  _serial_port = serial_port;
  reset_message_buffer();
}

bool ZcSerial::connect()
{
  _serial_port->begin(115200);
  return true;
}

void ZcSerial::wait_for_connection()
{
  /*
  while (!_ws.available())
  {
    _ws.poll();
    delay(10);
  }
  */
}

void ZcSerial::send(std::string message)
{
  if (_debug)
  {
    Serial.print("> ");
    Serial.println(message.c_str());
  }
  
  _serial_port->write('\02'); // STX
  _serial_port->write(message.c_str());
  _serial_port->write('\03'); // ETX
}

// Wait up to 2s for an inbound message with the supplied messageId
std::string ZcSerial::recv(int msgId)
{
  _waitingForMsgId = msgId;

  if (_pendingRecvMessage == "")
    _recvWaiting = true;

  unsigned long startTime = millis();
  while (millis() - startTime < 2000)
  {
    poll();
    if (_pendingRecvMessage != "")
    {
      std::string retval = _pendingRecvMessage;
      _pendingRecvMessage = "";
      _recvWaiting = false;
      return retval;
    }
    delay(10);
  }
  _recvWaiting = false;
  return "";
}

void ZcSerial::loop()
{
  poll();
}

void ZcSerial::disconnect() 
{
  _serial_port->write('\04'); //   EOT, causes the zc95 to stop a pattern if one is running, and reset the connection (clears any state, etc.)
}

void ZcSerial::process_message(std::string message)
{
  if (_debug)
  {
    Serial.print("< ");
    Serial.println(message.c_str());
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);
  JsonObject result = doc.as<JsonObject>();

  if (_recvWaiting && result.containsKey("MsgId") && result["MsgId"] == _waitingForMsgId)
  {
    _pendingRecvMessage = message;
    _recvWaiting = false;
  }
  else
  {
    if (_rcvQueue.size() < MAX_RECV_QUEUE_LENGTH)
      _rcvQueue.push(message);
    else if (_debug)
      Serial.println("Queue full, disgarding message"); // Not all programs will care about update messages, so this likely isn't an error
  }
}

void ZcSerial::poll()
{
  while (_serial_port->available() > 0)
  {
    uint8_t received_byte = _serial_port->read();
    if (!_stx_received)
    {
      if (received_byte == 0x02) // STX
      {
        _stx_received = true;
      }
    }
    else
    {
      if (_msg_buffer_pos < sizeof(_msg_buffer)-2)
      {
        if (received_byte == 0x03) // ETX
        {
          std::string message = _msg_buffer;
          process_message(message);
          reset_message_buffer();
        }
        else
        {
          _msg_buffer[_msg_buffer_pos++] = received_byte;
        }
        
      }
      else
      {
        // Buffer full, discard message and wait for next STX/message
        if (_debug) Serial.println("Warning: discarding incoming message due to length");
        reset_message_buffer();
      }
      
    }
    
  }
}

void ZcSerial::reset_message_buffer()
{
  memset(_msg_buffer, 0, sizeof(_msg_buffer));
  _msg_buffer_pos = 0;
  _stx_received = false;
}


