#include "ZcWs.h"

ZcWs::ZcWs(std::string ip, std::queue<std::string> &rcvQueue, bool debug) : _rcvQueue(rcvQueue)
{
  _debug = debug;
  _waitingForMsgId = 0;
  _recvWaiting = false;
  _ip = ip;
}

bool ZcWs::connect()
{
  _ws.onMessage(std::bind(&ZcWs::on_message, this, std::placeholders::_1));
  _ws.onEvent(std::bind(&ZcWs::on_event, this, std::placeholders::_1, std::placeholders::_2));
  if (_ws.connect(_ip.c_str(), 80, "/stream"))
  {
    Serial.println("Connected to WS");
    return true;
  }
  else
  {
    Serial.println("Failed to connect to WS");
    return false;
  }
}

void ZcWs::wait_for_connection()
{
  /*
  while (!_ws.available())
  {
    _ws.poll();
    delay(10);
  }
  */
}

void ZcWs::send(std::string message)
{
  if (_debug)
  {
    Serial.print("> ");
    Serial.println(message.c_str());
  }
  _ws.send(message.c_str());
  _next_ping_time = millis() + 4000;
}

// Wait up to 2s for an inbound message with the supplied messageId
std::string ZcWs::recv(int msgId)
{
  _waitingForMsgId = msgId;

  if (_pendingRecvMessage == "")
    _recvWaiting = true;

  unsigned long startTime = millis();
  while (millis() - startTime < 2000)
  {
    _ws.poll();
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

void ZcWs::loop()
{
  // The zc95 will close the connection after a few seconds of inactivity, so send
  // websocket pings if nothing else is being sent.
  if (_connected && millis() > _next_ping_time)
  {
    _ws.ping("");
    _next_ping_time = millis() + 4000;
  }

  _ws.poll();
}

void ZcWs::disconnect() { _ws.close(); }

bool ZcWs::is_connected() { return _connected; }

void ZcWs::on_message(websockets::WebsocketsMessage message)
{
  _next_ping_time = millis() + 4000;
  std::string msg = message.data().c_str();
  if (_debug)
  {
    Serial.print("< ");
    Serial.println(msg.c_str());
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  JsonObject result = doc.as<JsonObject>();

  if (_recvWaiting && result.containsKey("MsgId") && result["MsgId"] == _waitingForMsgId)
  {
    _pendingRecvMessage = msg;
    _recvWaiting = false;
  }
  else
  {
    if (_rcvQueue.size() < MAX_RECV_QUEUE_LENGTH)
      _rcvQueue.push(msg);
    else if (_debug)
      Serial.println("Queue full, disgarding message"); // Not all programs will care about update messages, so this likely isn't an error
  }
}

void ZcWs::on_event(websockets::WebsocketsEvent event, String data)
{
  if (event == websockets::WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connnection Opened");
    _next_ping_time = millis() + 4000;
    _connected = true;
  }
  else if (event == websockets::WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connnection Closed");
    _connected = false;
  }
  else if (event == websockets::WebsocketsEvent::GotPing)
  {
    Serial.println("Got a Ping!");
  }
  else if (event == websockets::WebsocketsEvent::GotPong)
  {
    Serial.println("Got a Pong!");
  }
}

void ZcWs::on_error(websockets::WebsocketsMessage error)
{
  /*
  std::string errMsg = error.data();
  if (errMsg != "None" && errMsg.length() > 1)
  {
    Serial.println("Websocket error: " + errMsg);
  }
  */
}
