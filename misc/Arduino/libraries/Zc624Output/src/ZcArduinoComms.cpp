#include "ZcArduinoComms.h"

ZcArduinoComms::ZcArduinoComms(SPIClass *spi, TwoWire *two_wire)
{
  _spi = spi;
  _two_wire = two_wire;

  pinMode(SS, OUTPUT);
  _spi->beginTransaction(SPISettings(SPI_BAUD_RATE, MSBFIRST, SPI_MODE0));
}

ZcArduinoComms::~ZcArduinoComms() {}

uint8_t ZcArduinoComms::i2c_write(uint8_t address, uint8_t *buffer, size_t len, bool no_stop)
{
  _two_wire->beginTransmission(address);
  size_t retval = _two_wire->write(buffer, len);
  _two_wire->endTransmission(!no_stop);

  return (uint8_t)retval;
}

uint8_t ZcArduinoComms::i2c_read(uint8_t address, uint8_t *buffer, size_t len, bool no_stop)
{
  size_t recv_count = _two_wire->requestFrom(address, len, no_stop);
  if (recv_count != len)
  {
    return 0;
  }

  for (uint8_t n = 0; n < len; n++)
  {
    buffer[n] = _two_wire->read();
  }

  return recv_count;
}

void ZcArduinoComms::spi_write_blocking(uint8_t *src, size_t len)
{
  digitalWrite(SS, LOW);
  delayMicroseconds(10);
  for (size_t n = 0; n < len; n++)
  {
    _spi->transfer(src[n]);
  }
  delayMicroseconds(10);
  digitalWrite(SS, HIGH);
  delayMicroseconds(10);
}
