#ifndef ZC_CONTROL_H
#define ZC_CONTROL_H

#include "ZcConnection.h"
#include "ZcMessaging.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <map>
#include <queue>
#include <string>

class ZcControl
{
public:
  ZcControl(std::string address, bool debug);
  ZcControl(HardwareSerial *serial_port, bool debug);
  ~ZcControl();

  bool connect();
  void loop();
  void disconnect();
  bool is_connected();

  bool start_pattern(std::string name);
  bool start_pattern(uint8_t pattern_id);
  void stop_pattern();

  void set_channel_power(uint8_t channel, uint16_t power_level);
  void set_channel_power(uint16_t power_level_chan1, uint16_t power_level_chan2, uint16_t power_level_chan3, uint16_t power_level_chan4);

  int get_front_panel_power(uint8_t channel);

private:
  ZcConnection *_zc_connection = NULL;
  ZcMessaging *_zc_messaging = NULL;
  std::queue<std::string> _rcv_queue;
  bool _debug;
  std::string _address;
  bool _connected = false;
  std::map<std::string, uint8_t> _pattern_map;
  uint16_t _channel_power[4] = {0};        // What each channel has been set to via this lib
  uint16_t _channel_pannel_power[4] = {0}; // What each channel is set to on the front pannel of the box (read only). 0-3 => channels 1-4

  void populate_patterns_map();
  void process_message(std::string message);
};

#endif
