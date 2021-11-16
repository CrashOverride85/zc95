/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "CControlsPortExp.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <string.h>

/*
 * Deal with port expander U7, which:
 *   - reads input from front pannel buttons A, B, C & D
 *   - is connected to IO1/2/3 on expansion header J17 (currently unused)
 *   - controls the LCD backlight
 */ 

CControlsPortExp::CControlsPortExp(uint8_t address)
{
    _address = address;
    _old_state = 3;
    _last_read = 0;    
    _button_states_at_last_check = 0;
    memset(_last_state_change, 0, sizeof(_last_state_change));
}

// Clear any pending input. has_button_been_pressed, etc., should return false after this has been called.
// Can't do this in the constructor as i2c won't have been initialised 
void CControlsPortExp::clear_input()
{
    int retval = i2c_read_timeout_us(i2c_default, _address, &_last_read, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CControlsPortExp::clear_intput i2c read error!\n");
    }

    _button_states_at_last_check = _last_read;
}

void CControlsPortExp::interrupt()
{
  _interrupt = true;
}

void CControlsPortExp::process(bool always_update)
{
  if (_interrupt || always_update)
  {
    _interrupt = false;
    uint8_t buffer[1];
    
    int retval = i2c_read_timeout_us(i2c_default, _address, buffer, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CControlsPortExp::process i2c read error!\n");
      return;
    }
    
    _last_read = buffer[0];
  }
}

bool CControlsPortExp::button_state(enum Button button)
{
    return (_last_read & (1 << (uint8_t)button));
}

bool CControlsPortExp::has_button_state_changed(enum Button button, bool *new_state)
{
  bool button_state_changed;
  bool last_button_state = (_button_states_at_last_check & (1 << (uint8_t)button));
  *new_state = button_state(button);

  button_state_changed = (last_button_state != *new_state);

  if (button_state_changed)
  {
    if (time_us_64() - _last_state_change[(uint8_t)button] < (10 * 1000) ) // 10ms
      return false;
    else
      _last_state_change[(uint8_t)button] = time_us_64();
  }

  if (*new_state)
    _button_states_at_last_check |= (1 << (uint8_t)button);
  else
    _button_states_at_last_check &= ~(1 << (uint8_t)button);

  return button_state_changed;
}

void CControlsPortExp::set_lcd_backlight(bool on)
{
    const int BackLightPin = 7;

    if (on)
        _data_out |= (1 << BackLightPin);
    else
        _data_out &= ~(1 << BackLightPin);

    int retval = i2c_write_timeout_us(i2c_default, _address, &_data_out, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CControlsPortExp::set_lcd_backlight i2c write error! (%d)\n", retval);
      return;
    }
}
