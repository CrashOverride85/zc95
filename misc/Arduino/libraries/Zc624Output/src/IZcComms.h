#ifndef _IZCCOMMS_H
#define _IZCCOMMS_H

#include <inttypes.h>
#include <stdlib.h>

class IZcComms
{
public:
  IZcComms() {}
  virtual ~IZcComms() {}

  virtual uint8_t i2c_write(uint8_t address, uint8_t *buffer, size_t len, bool no_stop) = 0;
  virtual uint8_t i2c_read(uint8_t address, uint8_t *buffer, size_t len, bool no_stop) = 0;

  virtual void spi_write_blocking(uint8_t *src, size_t len) = 0;
};

#endif
