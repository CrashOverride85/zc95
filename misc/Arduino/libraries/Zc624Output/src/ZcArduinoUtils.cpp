#include "ZcArduinoUtils.h"

ZcArduinoUtils::ZcArduinoUtils(HardwareSerial *port) { _serial_port = port; }

ZcArduinoUtils::~ZcArduinoUtils() {}

void ZcArduinoUtils::sleep_ms(uint16_t delay_ms) { delay(delay_ms); }

void ZcArduinoUtils::debug(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  if (_serial_port)
    _serial_port->printf(format, args);

  va_end(args);
}
