// -----
// CRotEnc.h - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
//
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
//
// This work is licensed under a BSD 3-Clause style license,
// https://www.mathertel.de/License.aspx.
//
// More information on: http://www.mathertel.de/Arduino
// -----
// 18.01.2014 created by Matthias Hertel
// 16.06.2019 pin initialization using INPUT_PULLUP
// 10.11.2020 Added the ability to obtain the encoder RPM
// 29.01.2021 Options for using rotary encoders with 2 state changes per latch.
// 11.09.2022 Adapted for Raspberry Pico by CrashOverride85
// -----

#ifndef CRotEnc2_h
#define CRotEnc2_h

#include <inttypes.h>


class CRotEnc
{
public:
  enum class Direction {
    NOROTATION = 0,
    CLOCKWISE = 1,
    COUNTERCLOCKWISE = -1
  };

  enum class LatchMode {
    FOUR3 = 1, // 4 steps, Latch at position 3 only (compatible to older versions)
    FOUR0 = 2, // 4 steps, Latch at position 0 (reverse wirings)
    TWO03 = 3  // 2 steps, Latch at position 0 and 3 
  };

  // ----- Constructor -----
  CRotEnc(LatchMode mode = LatchMode::FOUR3);

  // retrieve the current position
  long getPosition();

  // simple retrieve of the direction the knob was rotated last time. 0 = No rotation, 1 = Clockwise, -1 = Counter Clockwise
  Direction getDirection();

  // adjust the current position
  void setPosition(long newPosition);

  // call this function every some milliseconds or by using an interrupt for handling state changes of the rotary encoder.
  void process(uint8_t a, uint8_t b);

  // Returns the time in microseconds between the current observed
  uint64_t getUsBetweenRotations() const;

  // Returns the RPM
  unsigned long getRPM();

  int8_t get_rotary_encoder_change();

private:
  int _pin1, _pin2; // Arduino pins used for the encoder.
  
  LatchMode _mode; // Latch mode from initialization

  volatile int8_t _oldState;

  volatile long _position;        // Internal position (4 times _positionExt)
  volatile long _positionExt;     // External position
  volatile long _positionExtPrev; // External position (used only for direction checking)

  uint64_t _positionExtTime;     // The time the last position change was detected.
  uint64_t _positionExtTimePrev; // The time the previous position change was detected.
  long _postion_at_last_check; // should generally be -1, 0, or 1
};

#endif

// End