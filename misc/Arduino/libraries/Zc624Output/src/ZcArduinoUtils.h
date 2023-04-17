#ifndef _ZCARDUINOUTILS_H
#define _ZCARDUINOUTILS_H

#include "IZcUtils.h"
#include <inttypes.h>
#include <stdlib.h>

#include <Arduino.h>

class ZcArduinoUtils : public IZcUtils
{
public:
  ZcArduinoUtils(HardwareSerial *port);
  ~ZcArduinoUtils();

  void sleep_ms(uint16_t delay_ms);
  void debug(const char *format, ...);

private:
  HardwareSerial *_serial_port;
};

#endif
