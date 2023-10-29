#include "ZcArduinoUtils.h"

ZcArduinoUtils::ZcArduinoUtils(HardwareSerial *port) { _serial_port = port; }

ZcArduinoUtils::~ZcArduinoUtils() {}

void ZcArduinoUtils::sleep_ms(uint16_t delay_ms) { delay(delay_ms); }

void ZcArduinoUtils::debug(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  if (_serial_port)
  {
    // It should be possible to do "_serial_port->vprintf(format, args);", but the 
    // arduino serial/stream/print library has omitted vprintf for some reason.
    
    char buffer[200] = {0};
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    _serial_port->print(buffer);
  }

  va_end(args);
}
