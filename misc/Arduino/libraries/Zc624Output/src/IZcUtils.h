#ifndef _IZCUTILS_H
#define _IZCUTILS_H

#include <inttypes.h>
#include <stdlib.h>

class IZcUtils
{
public:
  IZcUtils() {}
  virtual ~IZcUtils() {}

  virtual void sleep_ms(uint16_t delay_ms) = 0;
  virtual void debug(const char *format, ...) = 0;
};

#endif
