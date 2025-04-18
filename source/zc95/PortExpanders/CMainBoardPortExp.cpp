/*
 * ZC95
 * Copyright (C) 2025  CrashOverride85
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

#include "../globals.h"
#include "../CUtil.h"
#include "hardware/gpio.h"
#include <string.h>
#include "CMainBoardPortExp.h"

/*
 * MKI (PCB <= v0.2)
 *   Deal with port expander U7 on the main board, which:
 *     - V0.1 front panel only: reads input from front panel buttons A, B, C & D.
 *       For front panels >= v0.2, p0-p3 are unused, and the buttons are connected 
 *       to a separate I/O expander on the front panel itself
 * 
 *     - is connected to IO1/2/3 on expansion header J17 (for optional audio input board)
 * 
 *     - controls the LCD backlight
 * 
 *    P4 - EXPAN_I03 - Mic pre-amp
 *    P5 - EXPAN_I02 - Mic power disable
 *    P6 - EXPAN_I01 - Audio enable (relay)
 *    P7 - LCD back light
 * 
 * MKII (PCB >= v2.0)
 *   Deal with port expander U28, which is connected to:
 *    P0 - TP4056 charger IC - standby
 *    P1 - TP4056 charger IC - charging
 *    P2 - N/C
 *    P3 - N/C
 *    P4 - Microphone preamp enable
 *    P5 - Microphone power enable (for electret mics)
 *    P6 - N/C
 *    P7 - N/C
 */

CMainBoardPortExp::CMainBoardPortExp(zc95_version_t hardware_version, IPortExpander* port_exp)
{
    _hardware_version = hardware_version;
    _port_exp = port_exp;

    _old_state = 3;
    _last_read = 0;    
    _button_states_at_last_check = 0;
    memset(_last_state_change, 0, sizeof(_last_state_change));
    clear_input();
    process(true);
}

// Clear any pending input. has_button_been_pressed, etc., should return false after this has been called.
void CMainBoardPortExp::clear_input()
{
    // Do init here too
    _port_exp->set_pin_as_output(MicPreampEnablePin);
    _port_exp->set_pin_as_output(MicPowerDisablePin);

    if (_hardware_version == zc95_version_t::MKI)
    {
        _port_exp->set_pin_as_output(AudioInputEnablePin);
        _port_exp->set_pin_as_output(BackLightPin);
    }
    
    _port_exp->read_port_expander(&_last_read);
    _button_states_at_last_check = _last_read;
}

void CMainBoardPortExp::interrupt()
{
    _interrupt = true;
}

void CMainBoardPortExp::process(bool always_update)
{
    if (_interrupt || always_update)
    {
        _interrupt = false;
        _port_exp->read_port_expander(&_last_read);
    }
}

bool CMainBoardPortExp::button_state(enum Button button)
{
    return (_last_read & (1 << (uint8_t)button));
}

bool CMainBoardPortExp::has_button_state_changed(enum Button button, bool *new_state)
{
  bool button_state_changed;
  bool last_button_state = (_button_states_at_last_check & (1 << (uint8_t)button));
  *new_state = button_state(button);

  button_state_changed = (last_button_state != *new_state);

  if (button_state_changed)
  {
    if (time_us_64() - _last_state_change[(uint8_t)button] < (25 * 1000) ) // 25ms debounce
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

/*
 * Enable microphone pre-amp on audio input board. Massively increases gain. 
 * Also results in mono input only as there is only one mic preamp
 */
void CMainBoardPortExp::mic_preamp_enable(bool enable)
{
    _port_exp->set_pin_state(MicPreampEnablePin, enable);
}

/*
 * When enabled and audio_input (below) is also enabled, ~3v is supplied via a current 
 * limiting resistor to the ring of the 3.5mm socket to power electret microphones
 */
void CMainBoardPortExp::mic_power_enable(bool enable)
{
    _port_exp->set_pin_state(MicPowerDisablePin, !enable);
}

/* Applies to MKI only where there's a single shared port for audio and serial.
 * MK2 has separate audio and serial ports so there is no switching.
 * Enabled  = 3.5mm socket used for Audio input
 * Disabled = 3.5mm socket used for RS232 serial
 */
void CMainBoardPortExp::audio_input_enable(bool enable)
{
    if (_hardware_version == zc95_version_t::MKI)
    {
        if (enable)
            printf("Enabling audio input\n");
        else
            printf("Disabling audio input\n");

        _port_exp->set_pin_state(AudioInputEnablePin, enable);
    }
}

// MKI only. On MKII backlight is connected directly to Pico
void CMainBoardPortExp::set_lcd_backlight(bool on)
{
    if (_hardware_version == zc95_version_t::MKI)
        _port_exp->set_pin_state(BackLightPin, on);
}

bool CMainBoardPortExp::get_tp4056_charge_status()
{
    return _port_exp->get_pin_state(ChargePin);
}

bool CMainBoardPortExp::get_tp4056_standby_status()
{
    return _port_exp->get_pin_state(StandbyPin);
}
