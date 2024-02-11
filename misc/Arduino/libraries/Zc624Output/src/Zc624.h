#ifndef _IZC624_H
#define _IZC624_H

#include "IZcComms.h"
#include "IZcUtils.h"

#define ZC624_ADDR 0x10

// Expected API version of ZC624. E.g. Major=2, Minor=0 corresponds to FW1.7 (and possibly some later versions).
// This to avoid the version check failing when there isn't an exact match, but the F/W verion is still compatible.
#define ZC624_EXPECTED_MAJOR_VERSION 2
#define ZC624_MINIMUM_MINOR_VERSION  0

class Zc624
{
public:
  Zc624(IZcComms *comms, IZcUtils *utils);
  ~Zc624();

  enum zc624_channel
  {
    Channel1 = 0,
    Channel2 = 1,
    Channel3 = 2,
    Channel4 = 3
  };

  bool get_version(char *buffer, uint8_t buflen);
  bool get_major_minor_version(uint8_t *major, uint8_t *minor);
  bool check_zc624(bool wait);
  bool set_channel_isolation(bool on);
  bool is_version_acceptable();
  bool is_version_acceptable(uint8_t major, uint8_t minor);

  void pulse(zc624_channel channel, uint8_t pos_us, uint8_t neg_us);
  void set_freq(zc624_channel channel, uint16_t freq_hz);
  void set_pulse_width(zc624_channel channel, uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us);
  void on(zc624_channel channel);
  void off(zc624_channel channel);
  void set_power(zc624_channel channel, uint16_t power);

private:
  enum class i2c_reg_t
  {
    // Read only
    TypeLow = 0x00,
    TypeHigh = 0x01,
    VersionMajor = 0x02,
    VersionMinor = 0x03,

    OverallStatus = 0x0F,
    Chan0Status = 0x10,
    Chan1Status = 0x11,
    Chan2Status = 0x12,
    Chan3Status = 0x13,

    VerStrStart = 0x20,
    VerStrEnd = 0x34, //  20 character string

    TestVal = 0x40,

    // Read/write
    // starting at 0x80
    ChannelIsolation = 0x80
  };

  struct message
  {
    uint8_t command;
    uint8_t arg0;
    uint8_t arg1;
    uint8_t arg2;
  };

  enum class spi_command_t
  {
    SetPower = 1,
    Poll = 2,
    PowerDown = 3,
    
    SetFreq = 4,
    SetPulseWidth = 5,
    SwitchOn = 6,
    SwitchOff = 7,
    NoOp = 8,
    SetTestVal = 9,
    Pulse = 10
  };

  enum zc624_status
  {
    Startup = 0x00,
    Ready = 0x01,
    Fault = 0x02
  };

  bool get_i2c_register_range(i2c_reg_t reg, uint8_t *buffer, uint8_t size);
  bool get_i2c_register(i2c_reg_t reg, uint8_t *value);
  bool write_i2c_register(i2c_reg_t reg, uint8_t value);

  void send_spi_message(message msg);

  IZcComms *_comms;
  IZcUtils *_utils;
};

#endif
