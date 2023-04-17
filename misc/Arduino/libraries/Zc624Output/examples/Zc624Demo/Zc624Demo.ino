/*
 * Simple demo of using the Zc624 library to control the
 * ZC624 output module from an ESP32.
 *
 * This demo:
 *  - Reports the version of the connected ZC624
 *  - Enables all output for 4 seconds a different frequencies
 *  - Runs a crude "waves" pattern on channel 4 until powered off
 */

#include "Zc624.h"
#include "ZcArduinoComms.h"
#include "ZcArduinoUtils.h"

#include <SPI.h>

IZcComms *_zcComms;
IZcUtils *_zcUtils = new ZcArduinoUtils(&Serial);

Zc624 *_zc624;

#define I2C_SDA 16
#define I2C_SCL 17

void setup()
{
  Serial.begin(115200);

  SPIClass *vspi = new SPIClass(VSPI);

  // Output configured SPI pins for board
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);

  vspi->begin();
  Wire.begin(I2C_SDA, I2C_SCL);

  _zcComms = new ZcArduinoComms(vspi, &Wire);
  _zc624 = new Zc624(_zcComms, _zcUtils);
}

void loop()
{
  char version_buffer[40];
  uint8_t version_major;
  uint8_t version_minor;

  // Get version string from output module
  if (_zc624->get_version(version_buffer, sizeof(version_buffer)))
  {
    Serial.print("ZC624 Version string: ");
    Serial.println(version_buffer);
  }
  else
  {
    Serial.println("Error getting version string");
  }

  // get major/minor api version
  if (_zc624->get_major_minor_version(&version_major, &version_minor))
  {
    Serial.print("Major version: ");
    Serial.println(version_major);

    Serial.print("Minor version: ");
    Serial.println(version_minor);
  }
  else
  {
    Serial.println("Error getting major/minor version");
  }

  // Check the zc624 module is found, and has passed it's self test.
  // Passing wait=true to wait up to 2 seconds for it to become ready.
  bool is_ready = _zc624->check_zc624(true);
  if (!is_ready)
  {
    Serial.println("ERROR: Zc624 is not ready. stop.");
    while (1)
      ;
  }

  basic_mode_test();

  // Start waves pattern. This never returns.
  waves();
}

// This uses the set_freq/on/off commands to generate output, rather than
// sending individual pulses
void basic_mode_test()
{
  Serial.println("Outputting a 25, 50, 75 and 100hz signal on channels 1, 2, 3 "
                 "and 4 respectively at 25\% power for 4 seconds");

  // Set frequency. Valid values are 1 - 1000. But anything over ~250 is
  // probably a bad idea.
  _zc624->set_freq(Zc624::Channel1, 25);
  _zc624->set_freq(Zc624::Channel2, 50);
  _zc624->set_freq(Zc624::Channel3, 75);
  _zc624->set_freq(Zc624::Channel4, 100);

  // Set power. Valid values are 0-1000 for 0-100%
  _zc624->set_power(Zc624::Channel1, 250);
  _zc624->set_power(Zc624::Channel2, 250);
  _zc624->set_power(Zc624::Channel3, 250);
  _zc624->set_power(Zc624::Channel4, 250);

  // Set pulse width for positive and negative pulse. Valid values are 0-255
  _zc624->set_pulse_width(Zc624::Channel1, 100, 100);
  _zc624->set_pulse_width(Zc624::Channel2, 100, 100);
  _zc624->set_pulse_width(Zc624::Channel3, 100, 100);
  _zc624->set_pulse_width(Zc624::Channel4, 100, 100);

  Serial.println("Switching output on");

  // Enable output
  _zc624->on(Zc624::Channel1);
  _zc624->on(Zc624::Channel2);
  _zc624->on(Zc624::Channel3);
  _zc624->on(Zc624::Channel4);

  delay(4 * 1000);

  // Disable output
  Serial.println("Switching output off");
  _zc624->off(Zc624::Channel1);
  _zc624->off(Zc624::Channel2);
  _zc624->off(Zc624::Channel3);
  _zc624->off(Zc624::Channel4);
}

// Very crude waves implementation. Only outputs on channel 4 and varies pulse
// width. Blocks whilst waiting to send next pulse, so only suitable as a demo
// of the zc624->pulse() method
void waves()
{
  Serial.println("Starting waves on channel 4");
  const uint8_t min_pulse_width_us = 10;
  const uint8_t max_pulse_width_us = 150;

  const uint32_t min_gap_between_pulses_us = 5000;  //  5 ms / 200 hz
  const uint32_t max_gap_between_pulses_us = 20000; // 20 ms /  50 hz

  int16_t pulse_width_us = 10;
  bool pulse_width_increasing = true;

  uint32_t gap_between_pulses_us = 5000;
  bool gap_between_pulses_increasing = true;

  // 33% power
  _zc624->set_power(Zc624::Channel4, 333);

  while (1)
  {
    // Wait until time to generate next pulse
    delayMicroseconds(gap_between_pulses_us);

    // Generate symmetric pulses (both durations the same)
    _zc624->pulse(Zc624::Channel4, pulse_width_us, pulse_width_us);

    // Adjust pulse width
    if (pulse_width_increasing)
    {
      if (pulse_width_us++ > max_pulse_width_us)
      {
        pulse_width_us = max_pulse_width_us;
        pulse_width_increasing = false;
      }
    }
    else
    {
      if (pulse_width_us-- < min_pulse_width_us)
      {
        pulse_width_us = min_pulse_width_us;
        pulse_width_increasing = true;
      }
    }

    // Adjust delay between pulses (frequency)
    if (gap_between_pulses_increasing)
    {
      gap_between_pulses_us += 100;
      if (gap_between_pulses_us > max_gap_between_pulses_us)
      {
        gap_between_pulses_us = max_gap_between_pulses_us;
        gap_between_pulses_increasing = false;
      }
    }
    else
    {
      gap_between_pulses_us -= 100;
      if (gap_between_pulses_us < min_gap_between_pulses_us)
      {
        gap_between_pulses_us = min_gap_between_pulses_us;
        gap_between_pulses_increasing = true;
      }
    }
  }
}
