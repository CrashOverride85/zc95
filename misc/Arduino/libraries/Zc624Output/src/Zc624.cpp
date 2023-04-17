#include "Zc624.h"

#include "Arduino.h"

Zc624::Zc624(IZcComms *comms, IZcUtils *utils)
{
  _comms = comms;
  _utils = utils;
}

Zc624::~Zc624() {}

bool Zc624::get_i2c_register_range(i2c_reg_t reg, uint8_t *buffer, uint8_t size)
{
  uint8_t buf[1];
  buf[0] = (uint8_t)reg;

  int count = _comms->i2c_write(ZC624_ADDR, buf, 1, false);
  if (count < 0)
  {
    _utils->debug("get_i2c_register_range for addr=%d, reg=%d failed (write)\n", ZC624_ADDR, (uint8_t)reg);
    return false;
  }

  count = _comms->i2c_read(ZC624_ADDR, buffer, size, false);
  if (count != size)
  {
    _utils->debug("get_i2c_register_range for addr=%d, reg=%d failed (read; "
                  "size = %d, read count = %d)\n",
                  ZC624_ADDR, (uint8_t)reg, size, count);
    return false;
  }

  return true;
}

bool Zc624::get_i2c_register(i2c_reg_t reg, uint8_t *value)
{
  uint8_t buf[1] = {0};
  bool retval = get_i2c_register_range(reg, buf, 1);
  *value = buf[0];
  return retval;
}

bool Zc624::write_i2c_register(i2c_reg_t reg, uint8_t value)
{
  uint8_t buf[2];
  buf[0] = (uint8_t)reg;
  buf[1] = value;

  int count = _comms->i2c_write(ZC624_ADDR, buf, sizeof(buf), false);
  if (count != sizeof(buf))
  {
    _utils->debug("write_i2c_register for addr=%d, reg=%d, value=%d, "
                  "bytes_written=%d (of %d) failed (write address)\n",
                  ZC624_ADDR, (uint8_t)reg, value, count, sizeof(buf));
    return false;
  }

  return true;
}

void Zc624::send_spi_message(message msg)
{
  uint8_t *ptr = (uint8_t *)&msg;
  _comms->spi_write_blocking(ptr, sizeof(msg));
}

bool Zc624::get_version(char *buffer, uint8_t buflen)
{
  uint8_t bytes_to_read = ((uint8_t)i2c_reg_t::VerStrEnd - (uint8_t)i2c_reg_t::VerStrStart);
  if (bytes_to_read > (buflen - 1))
    bytes_to_read = buflen - 1;

  bool retval = get_i2c_register_range(i2c_reg_t::VerStrStart, (uint8_t *)buffer, bytes_to_read);
  buffer[buflen - 1] = '\0';

  return retval;
}

bool Zc624::get_major_minor_version(uint8_t *major, uint8_t *minor)
{
  bool retval = true;
  retval &= get_i2c_register(i2c_reg_t::VersionMajor, major);
  retval &= get_i2c_register(i2c_reg_t::VersionMinor, minor);

  return retval;
}

bool Zc624::check_zc624(bool wait)
{
  uint8_t status = 0;
  if (!get_i2c_register(i2c_reg_t::OverallStatus, &status))
  {
    _utils->debug("Failed to read OverallStatus register from ZC624\n");
    return false;
  }

  // Wait for upto 2 seconds for ZC624 to become ready
  uint8_t count = 0;
  do
  {
    if (!get_i2c_register(i2c_reg_t::OverallStatus, &status))
    {
      _utils->debug("Failed to read OverallStatus register from ZC624\n");
      return false;
    }
    count++;

    if (status != zc624_status::Startup)
      break;

    _utils->sleep_ms(100);

  } while (wait && count < 20);

  if (status != zc624_status::Ready)
  {
    _utils->debug("ZC624 is not ready (status = %d)\n", status);
    return false;
  }

  return true;
}

// Enable/disable channel isolation. This setting does have some safety
// implications, so try and confirm that it does get set.
bool Zc624::set_channel_isolation(bool on)
{
  bool success = write_i2c_register(i2c_reg_t::ChannelIsolation, on);
  if (!success)
  {
    _utils->debug("failed to set channel isolation! (write)\n");
    return false;
  }

  // Read back value to double check it got set ok
  uint8_t value = 0xFF;
  success = get_i2c_register(i2c_reg_t::ChannelIsolation, &value);
  if (!success || (value != on))
  {
    _utils->debug("failed to set channel isolation! (read back: success=%d, value=%d)\n", success, value);
    return false;
  }

  return true;
}

void Zc624::pulse(zc624_channel channel, uint8_t pos_us, uint8_t neg_us)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::Pulse;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = pos_us;
  msg.arg2 = neg_us;

  send_spi_message(msg);
}

void Zc624::set_freq(zc624_channel channel, uint16_t freq_hz)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::SetFreq;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = (freq_hz >> 8) & 0xFF;
  msg.arg2 = freq_hz & 0xFF;

  send_spi_message(msg);
}

void Zc624::set_pulse_width(zc624_channel channel, uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::SetPulseWidth;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = pulse_width_pos_us;
  msg.arg2 = pulse_width_neg_us;

  send_spi_message(msg);
}

void Zc624::on(zc624_channel channel)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::SwitchOn;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = 0;
  msg.arg2 = 0;

  send_spi_message(msg);
}

void Zc624::off(zc624_channel channel)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::SwitchOff;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = 0;
  msg.arg2 = 0;

  send_spi_message(msg);
}

// power should be 0-1000
void Zc624::set_power(zc624_channel channel, uint16_t power)
{
  message msg;

  msg.command = (uint8_t)spi_command_t::SetPower;
  msg.arg0 = (uint8_t)channel;
  msg.arg1 = (power >> 8) & 0xFF;
  msg.arg2 = power & 0xFF;

  send_spi_message(msg);
}
