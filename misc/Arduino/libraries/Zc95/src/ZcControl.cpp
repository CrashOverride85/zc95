#include "ZcControl.h"
#include "ZcWs.h"

ZcControl::ZcControl(std::string address, bool debug)
{
  _zc_connection = new ZcWs(address, _rcv_queue, debug);
  _zc_messaging = new ZcMessaging(_zc_connection, debug);
}

ZcControl::~ZcControl()
{
  if (_zc_messaging)
  {
    delete _zc_messaging;
    _zc_messaging = NULL;
  }

  if (_zc_connection)
  {
    delete _zc_connection;
    _zc_connection = NULL;
  }
}

bool ZcControl::connect()
{
  if (_zc_connection->connect())
  {
    return true;
  }

  return false;
}

void ZcControl::loop()
{
  _zc_connection->loop();

  bool is_connected = _zc_connection->is_connected();
  if (_connected != is_connected)
  {
    // On connection, get a list of available patterns
    if (is_connected)
    {
      populate_patterns_map();
    }

    _connected = _zc_connection->is_connected();
  }

  // Process any received messages
  while (!_rcv_queue.empty())
  {
    std::string message = _rcv_queue.front();
    _rcv_queue.pop();
    process_message(message);
  }
}

void ZcControl::disconnect() { _zc_connection->disconnect(); }

bool ZcControl::start_pattern(std::string name)
{
  if (!_connected)
  {
    Serial.println("ZcControl::start_pattern(): not connected");
    return false;
  }

  std::map<std::string, uint8_t>::iterator it = _pattern_map.find(name);
  if (it == _pattern_map.end())
  {
    Serial.print("ZcControl::start_pattern(): Pattern [");
    Serial.print(name.c_str());
    Serial.println("] not found");
    return false;
  }

  return start_pattern(it->second);
}

bool ZcControl::start_pattern(uint8_t pattern_id)
{
  if (!_connected)
  {
    Serial.println("ZcControl::start_pattern(): not connected");
    return false;
  }

  _zc_messaging->PatternStart(pattern_id);
  return true;
}

void ZcControl::stop_pattern() { _zc_messaging->SendPatternStopMessage(); }

bool ZcControl::is_connected() { return _connected; }

void ZcControl::set_channel_power(uint8_t channel, uint16_t power_level)
{
  if (channel == 0 || channel > 4)
  {
    Serial.println("ZcControl::set_channel_power(): invalid channel");
    return;
  }

  _channel_power[channel - 1] = power_level;

  _zc_messaging->SendSetPowerMessage(_channel_power[0], _channel_power[1], _channel_power[2], _channel_power[3]);
}

void ZcControl::set_channel_power(uint16_t power_level_chan1, uint16_t power_level_chan2, uint16_t power_level_chan3,
                                  uint16_t power_level_chan4)
{
  _channel_power[0] = power_level_chan1;
  _channel_power[1] = power_level_chan2;
  _channel_power[2] = power_level_chan3;
  _channel_power[3] = power_level_chan4;
  _zc_messaging->SendSetPowerMessage(_channel_power[0], _channel_power[1], _channel_power[2], _channel_power[3]);
}

int ZcControl::get_front_panel_power(uint8_t channel)
{
  if (channel < 1 || channel > 4)
  {
    Serial.println("Error: unexpected channel number!");
    return -1;
  }

  return _channel_pannel_power[channel - 1];
}

///////////////////// Private /////////////////////

void ZcControl::populate_patterns_map()
{
  JsonArray patterns = _zc_messaging->GetPatterns();
  _pattern_map.clear();
  for (JsonObject pattern : patterns)
  {
    int id = pattern["Id"];
    std::string name = pattern["Name"];

    Serial.print(id);
    Serial.print(" - ");
    Serial.println(name.c_str());

    _pattern_map.insert(std::pair<std::string, uint8_t>(name, id));
  }
}

void ZcControl::process_message(std::string message)
{
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, message);

  JsonObject result = doc.as<JsonObject>();

  if (result == JsonObject() || !result.containsKey("Type"))
  {
    if (_debug)
    {
      Serial.println("Error processing received message");
    }
    return;
  }

  if (strcmp(result["Type"], "PowerStatus") == 0)
  {
    JsonArray channels = result["Channels"].as<JsonArray>();

    for (JsonVariant channel : channels)
    {
      int channel_number = channel["Channel"];
      int front_pannel_power = channel["PowerLimit"];

      if (channel_number < 1 || channel_number > 4)
      {
        Serial.println("Error: unexpected channel number!");
        break;
      }

      _channel_pannel_power[channel_number - 1] = front_pannel_power;
    }
  }
  else
  {
    if (_debug)
    {
      std::string type = result["Type"];
      Serial.print("Ignoring currently unhandled message type: ");
      Serial.println(type.c_str());
    }
  }
}
