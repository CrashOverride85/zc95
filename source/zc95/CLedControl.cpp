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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "CLedControl.h"

/*
 * Control the 6 ws2812 LEDs on the front pannel. Based on this code:
 *
 * ws2812 control code based on example which is:
 *      Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 *      SPDX-License-Identifier: BSD-3-Clause
 */


CLedControl::CLedControl(uint8_t tx_pin, CSavedSettings *settings)
{
    _tx_pin = tx_pin;
    _pio = pio0;
    _sm = 0;
    pio_sm_claim(_pio, _sm);
    _settings = settings;
    _brightness = get_led_brightness();;
}

CLedControl::~CLedControl()
{
    pio_sm_set_enabled(_pio, _sm, false);
    pio_sm_unclaim(_pio, _sm);
}

void CLedControl::put_pixel(uint32_t pixel_rgb) 
{
    pio_sm_put_blocking(_pio, _sm, pixel_rgb << 8u);
}

uint8_t CLedControl::get_led_brightness()
{
    if (_settings != NULL)
        return _settings->get_led_brightness();
    else
        return 10;
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) 
{
    return
            ((uint32_t) (r) << 16) |
            ((uint32_t) (g) << 8)  |
            (uint32_t)  (b);            
}

void CLedControl::init()
{
    memset((void*)_led_state, 0, sizeof(_led_state));
    _led_state_changed = true;

    uint offset = pio_add_program(_pio, &ws2812_program);
    ws2812_program_init(_pio, _sm, offset, _tx_pin, 800000, false);
}

void CLedControl::loop(bool force_update)
{
    if (get_led_brightness() != _brightness)
    {
        _led_state_changed = true;
        _brightness = get_led_brightness();
    }

    if (_led_state_changed || force_update)
    {
        update_leds();
    }
}

void CLedControl::update_leds()
{
    // Not sure what the min refresh time for the LEDs is,
    // but it seems to be somewhere between 100us and 1000us
    if (time_us_64() - _last_led_update > 1000) // 1ms
    {
        for (uint8_t n = 0; n < LED_COUNT ; n++)
        {
            put_pixel(get_brightness_adjusted_led_colour(n));
        }

        _last_led_update  = time_us_64();
        _led_state_changed = false;
    }
    else
    {
        // It's to soon to update LEDs again (LEDs won't update / will ignore
        // the update), but make sure it happens next loop
        _led_state_changed = true;
    }
}

// Set an LED colour & brightness (0-100)
uint32_t CLedControl::get_brightness_adjusted_led_colour(uint8_t led)
{
    uint32_t colour = _led_state[led];
    uint8_t r = (colour & 0xFF0000) >> 16;
    uint8_t g = (colour & 0x00FF00) >> 8;
    uint8_t b = (colour & 0xFF);

    if (_brightness < 1)
        _brightness = 0;
    if (_brightness > 100)
        _brightness = 100;

    uint8_t r_adj = (int)((float)r * (float)_brightness/(float)100);
    uint8_t g_adj = (int)((float)g * (float)_brightness/(float)100);
    uint8_t b_adj = (int)((float)b * (float)_brightness/(float)100);

    // For any values < 0, round up to 1 if not passed in as 0
    if (r > 0 && r_adj == 0) r_adj = 1;
    if (g > 0 && g_adj == 0) g_adj = 1;
    if (b > 0 && b_adj == 0) b_adj = 1;

    return urgb_u32(r_adj, g_adj, b_adj);
}

// Set an LED colour
void CLedControl::set_led_colour(LED led, uint32_t colour)
{
    if (_led_state[(uint8_t)led] != colour)
    {
        _led_state[(uint8_t)led] = colour; 
        _led_state_changed = true;
    }
}

void CLedControl::set_all_led_colour(uint32_t colour)
{
    set_led_colour(LED::Channel1, colour);
    set_led_colour(LED::Channel2, colour);
    set_led_colour(LED::Channel3, colour);
    set_led_colour(LED::Channel4, colour);
    set_led_colour(LED::Trigger1, colour);
    set_led_colour(LED::Trigger2, colour);
}
