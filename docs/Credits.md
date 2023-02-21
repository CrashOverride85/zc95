## Credits

The ZC95 is built on top of many great libraries and bits of other code. Hopefully I've managed to list all of them here.

In addition to code, the hardware - in particular the estim output section - draws very heavily on the design of the MK312-BT.

----

### Library/code:
hagl & hagl_pico_mipi - Hardware Agnostic Graphics Library & Pico HAL

### License:
    MIT License

    Copyright (c) 2021 Mika Tuupola

### URL:
https://github.com/tuupola/hagl

https://github.com/tuupola/hagl_pico_mipi

### Use:
Everywhere! - Display driver

----

### Library/code:
ESP32 FFT

### License:
MIT License

    This code was written by [Robin Scheibler](http://www.robinscheibler.org) during rainy days in October 2017.
    "Classified" for ESP and ARduino by M. Steltman Mey 2021

### URL:
https://github.com/yash-sanghvi/ESP32/issues/1

https://github.com/yash-sanghvi/ESP32/files/6474828/ESPfft.zip

### Use:
Spectrum analyzer in the "Audio Threshold" pattern

----

### Library/code:
picow-wlan-setup-webinterface

### License:
    * Author: Floris Bos
    * ### License: public domain / unlicense

### URL:
https://github.com/maxnet/picow-wlan-setup-webinterface
 
### Use:
Forms the base of the WiFi AP mode and configuration, including QR code and Web interface to set SSID/PSK. Whilst not a library, large parts (including the web interface) are mostly used as-is

----

### Library/code: 
LWIP HTTP server implementation

### License:
BSD-3-Clause

    * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
    * All rights reserved.

    * Author: Adam Dunkels <adam@sics.se>
    *         Simon Goldschmidt

### Use:
HTTP server used both when in AP mode for WiFi setup, and when connected to Wifi for the websocket server. Modifications:
 * In AP mode, returns a "HTTP/1.1 511 Network Authentication Required" response which causes phones (only tested on an iPhone so far) to automatically display the web config page on connecting, instead of having to either enter an address, or browse to a non-https page first.
 * Websocket support - see below

----

### Library/code: 
esp-httpd - for websocket support

### License:
As per lwIP httpd

### URL:
https://github.com/lujji/esp-httpd

### Use:
Websocket server when connected to WiFi. Websocket code extracted and patched into the Pico version of the httpd server, then modified to support websocket pings (opcode 0x09).

----

### Library/code: 
RFC 1521 base64 encoding/decoding
    
### License:
    *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
    *  SPDX-License-Identifier: Apache-2.0
 
### Use:
Websocket implementation

----

### Library/code: 
sha1

### License:
BSD-3-Clause
    
     Copyright (c) 2011, Micael Hildenborg
     All rights reserved.
 
### URL:
https://github.com/daviswalker/smallsha1
 
### Use:
Websocket implementation

----

### Library/code: 
QR Code generator library (C)

### License:
    Copyright (c) Project Nayuki. (MIT License)
### URL:
https://www.nayuki.io/page/qr-code-generator-library

### Use:
Generating QR code used to provide network details when in AP mode for WiFi setup

----

### Library/code: 
dhcpserver - part of the MicroPython project

### License:
    * The MIT License (MIT)
    * Copyright (c) 2018-2019 Damien P. George

### Use:
DHCP server used when running in AP mode

----

### Library/code: 
dnsserver 

### License:
    * The MIT License (MIT)
    * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
  
### Use:
DNS server used when running in AP mode
    
----

### Library/code:
ArduinoJson

### License:
The MIT License (MIT)

    Copyright © 2014-2022, Benoit BLANCHON

### URL:
https://arduinojson.org/
 
### Use:
Serializing and deserializing JSON based websocket messages

----

### Library/code: 
Lua 5.1.5

### License:
    MIT License

    Copyright (C) 1994-2012 Lua.org, PUC-Rio.  All rights reserved.

### URL:
https://www.lua.org
 
### Use:
Running uploaded Lua scripts

----

### Library/code: 
Lua CMake files

### License:
?

### URL:
https://github.com/walterschell/Lua
 
### Use:
Building Lua with CMake. Note above URL includes Lua 5.4.4, but has been tweaked to build Lua 5.1.5

----

### Library/code: 
Lua-5.1.5-TR - Lua 5.1.5 with tiny ram patch
    
### License:
    MIT License
    Copyright (c) 2021 Vincent Hamp

### URL:
https://github.com/higaski/Lua-5.1.5-TR
 
### Use:
Running uploaded Lua scripts


