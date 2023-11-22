# Arduino control of ZC95

## Introduction
This is a library to control a ZC95 box from an Arduino (tested on ESP32) using either a Serial (RS-232) or WiFi/Websocket connection. At present it is fairly limited, and only allows:
* Start/Stop pattern
* Set power level for each channel (up to the maximum set on the front panel)
* Read the power level set on the front panel for each chanel

## Connection

### Serial
To connect to an ESP32, a RS232 <> 3.3v/TTL transceiver chip or module is needed; a module based around a MAX3232 is perfect.

The 3.5mm TRS "Aux" socket must be used (not Accessory), which has this pinout:
* Tip = Transmit (ZC95 output)
* Ring = Receive
* Sleeve = Ground

To enable & start serial remote access mode on the ZC95, see the [remote access][SerialRemoteAccess] notes. 

There is an example Arduino sketch for serial control that connects using Serial2, starts the Waves pattern, and sets channel 1 to 50%, waits 10 seconds, then stops the pattern.

### WiFi
Configure the ZC95 to connect to a WiFi network, then connect to that network by following the [WiFi remote access][WiFiRemoteAccess] notes.

There is an example Arduino sketch for WiFi control that starts the Waves pattern, and sets channel 1 to 50%, waits 10 seconds, then stops the pattern.
Before it can be used, fill in the SSID, Password, and IP (as shown on the ZC95 display) in the `ConnectionCredentials.h` file.

## Methods

Regardless of connection method, these methods are available in the ZcControl class:

---

`bool connect()`

Connect to ZC95 using method passed into the constructor.

---

`void disconnect()`

Disconnect from ZC95. For serial, closes the serial port and causes the ZC95 to stop anything running, and clear any state.

---
`void loop()`

Should be called frequently, and manages connection, disconnection, processing of incoming messages, etc.

---
`bool is_connected()`

Returns true if connected to ZC95, false otherwise.

---
`void wait_for_connection()`

Waits (indefinitely) for a connection. Currently this doesn't do anything clever, like making multiple connection attempts or timing out - so probably only useful for quick tests. 

---
`bool start_pattern(std::string name)`

Starts pattern named `<name>`, e.g. "Waves".

---
`bool start_pattern(uint8_t pattern_id)`

Starts pattern with id `pattern_id`. A list of pattern IDs can be found by running the [pattern_list.py][PatternListPy] script. 

---
`void stop_pattern()`

Stops the running pattern.

---
`void set_channel_power(uint8_t channel, uint16_t power_level)`

Sets the power level (0 - 1000) for the specified channel (1 - 4).

Note that the power level set here is limited to the front panel power setting.

---
`void set_channel_power(uint16_t power_level_chan1, uint16_t power_level_chan2, uint16_t power_level_chan3, uint16_t power_level_chan4)`

Sets the power level (0 - 1000) for all channels.

Note that the power level set here is limited to the front panel power setting.

---
`int get_front_panel_power(uint8_t channel)`

Gets the power level set on the front panel (0 - 1000).

---


[SerialRemoteAccess]: ../../../../docs/RemoteAccess.md#serial-control
[WiFiRemoteAccess]: ../../../../docs/RemoteAccess.md#connecting-using-wifi
[PatternListPy]: ../../../../docs/RemoteAccess.md#pattern_listpy
