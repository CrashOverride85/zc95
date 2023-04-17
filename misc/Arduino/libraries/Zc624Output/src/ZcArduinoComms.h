#ifndef _ZCARDUINOCOMMS_H
#define _ZCARDUINOCOMMS_H

#include <inttypes.h>
#include <stdlib.h>

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "IZcComms.h"

#define SPI_BAUD_RATE 2000000 // 2mhz

class ZcArduinoComms : public IZcComms
{
public:
  ZcArduinoComms(SPIClass *spi, TwoWire *two_wire = &Wire);
  ~ZcArduinoComms();

  uint8_t i2c_write(uint8_t address, uint8_t *buffer, size_t len, bool no_stop);
  uint8_t i2c_read(uint8_t address, uint8_t *buffer, size_t len, bool no_stop);

  void spi_write_blocking(uint8_t *src, size_t len);

private:
  SPIClass *_spi;
  TwoWire *_two_wire;
};

#endif
