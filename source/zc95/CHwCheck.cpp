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

#include "hardware/adc.h"

#include <font6x9.h>
#include "CHwCheck.h"
#include "i2c_scan.h"
#include "config.h"
#include "CHwCheckZc624.h"

/*
 * Check for the presence of all expected i2c devices. If any are missing, flash the LEDs and try to display an 
 * error on screen indicating what's missing. The backlight pin of the display is connected to the U7 port
 * expander, so if that's not working, the display might not be visible. 
 */

CHwCheck::CHwCheck()
{
    _devices.push_front(device(EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U8)", "Port exp U8"));
    _devices.push_front(device(CONTROLS_PORT_EXP_ADDR, "Port expander for buttons (U7)", "Port exp U7"));
    _devices.push_front(device(EEPROM_ADDR, "EEPROM (read)", "EEPROM"));
    _devices.push_front(device(EEPROM_ADDR+1, "EEPROM (write)", "EEPROM"));
    _devices.push_front(device(ZC624_ADDR, "ZC624 output board", "ZC624"));
    
    // these two ICs are on the front panel
    _devices.push_front(device(ADC_ADDR, "Front pannel ADC", "FP ADC U1"));
    _devices.push_front(device(FP_ANALOG_PORT_EXP_2_ADDR, "Front pannel port expander (U2)", "FP Port Exp U2"));

    _last_update = 0;
}

void CHwCheck::check_part1()
{
    bool ok = true;
    int ret;
    uint8_t rxdata;

    enum Cause cause = Cause::UNKNOWN;

    printf("\n\nHardware check (part1)\n");
    printf("======================\n");
    
    // Check battery isn't flat
    if (get_battery_voltage() < 10.5)
    {
        printf("Battery is flat!\n");
        ok = false;
        cause = Cause::BATTERY;
    }

    printf("I2C scan:\n");
    i2c_scan::scan(i2c0);
    printf("\n");
    std::list<device>::iterator it;
    for (it = _devices.begin(); it != _devices.end(); ++it)
    {
        printf("    %s...", it->description.c_str());
        ret = i2c_read_blocking(i2c0, it->address, &rxdata, 1, false);
        if (ret >= 0)
        {
            printf("Ok\n");
            it->present = true;
        }
        else
        {
            printf("NOT FOUND! (expected on address %d)\n", it->address);
            cause = Cause::MISSING;
            ok = false;
        }
    }
    if (ok)
    {
        printf("Status: Ok\n\n");
    }
    else
    {
        printf("Status: FAILED\n\n");
        CLedControl led = CLedControl(PIN_LED, NULL);
        led.init();
        hw_check_failed(cause, &led, NULL); // this never returns
    }
}

// The ZC624 output board takes a while to initialize from power on, so check its status much later when it should be ready.
// By the time this is called, the display and LEDs should be initialized, so need these passed in (CControlsPortExp controls display backlight)
void CHwCheck::check_part2(CLedControl *ledControl, CControlsPortExp *controls)
{
    printf("\n\nHardware check (part2)\n");
    printf("======================\n");

    printf("    ZC624 Version = [%s]\n", _checkZc624.get_version().c_str());
    printf("    ZC624 status...");
    if (!_checkZc624.check_zc624())
    {
        printf("FAULT\n");
        hw_check_failed(Cause::ZC628, ledControl, controls); // this never returns
    }
    printf("Ok\n\n");
}

void CHwCheck::show_error_text_message(int y, std::string message)
{
    y += 2;
    put_text(message, 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));
}

void CHwCheck::show_error_text_missing(int y)
{
    y += 2;
    put_text("Missing:", 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));
    
    for (std::list<device>::iterator it = _devices.begin(); it != _devices.end(); ++it)
    {
        if (!it->present)
        {
            put_text("   * " + it->display + "\n", 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));
        }
    }    
}

void CHwCheck::hw_check_failed(enum Cause casue, CLedControl *ledControl, CControlsPortExp *controls)
{
    int y = 0;
    ledControl->set_all_led_colour(LedColour::Red);
    ledControl->loop();

    if (controls == NULL)
    {  
        hagl_init();
    }
    else
    {
        controls->set_lcd_backlight(true);
    }

    hagl_clear_screen();

    put_text("Hardware check failed", (y++ * 10), 10, hagl_color(0xFF, 0xFF, 0xFF));

    switch (casue)
    {
        case Cause::MISSING:
            show_error_text_missing(y);
            break;

        case Cause::BATTERY:
            show_error_text_message(y, "Battery is flat!");
            break;

        case Cause::ZC628:
            show_error_text_message(y, "ZC628 (output) fault");
            break;
        
        default:
            show_error_text_message(y, "Unknown error");
            break;
    }

    hagl_flush();

    printf("HALT\n");
    while(1)
    {
        sleep_ms(1000);
        ledControl->set_all_led_colour(LedColour::Black);
        ledControl->loop();
        
        sleep_ms(1000);
        ledControl->set_all_led_colour(LedColour::Red);
        ledControl->loop();
    };
}

std::string CHwCheck::get_zc624_version()
{
    return _checkZc624.get_version();
}

void CHwCheck::put_text(std::string text, int16_t x, int16_t y, color_t color)
{
    if (text == "")
        text = " ";

    std::wstring widestr = std::wstring(text.begin(), text.end());
    hagl_put_text(widestr.c_str(), x, y, color, font6x9);
}


int CHwCheck::cmpfunc (const void *a, const void *b)
{
   return ( *(uint32_t*)a - *(uint32_t*)b );
}

int CHwCheck::cmpfunc_uint8_t (const void *a, const void *b)
{
   return ( *(uint8_t*)a - *(uint8_t*)b );
}

float CHwCheck::get_adc_voltage()
{
    uint32_t readings[10];
    const float conversion_factor = 3.3f / (1 << 12);

    adc_select_input(0);

    // get 10 readings
    for (uint8_t reading=0; reading < 10; reading++)
        readings[reading] = adc_read();

    // ignore 2 highest and 2 lowest values. get the average of the rest
    qsort(readings, 10, sizeof(uint32_t), CHwCheck::cmpfunc);
    uint32_t total=0;
    for (uint8_t reading=2; reading < 8; reading++)
        total += readings[reading];
    
    uint32_t avg = total/6;
    
    // Convert to voltage and return
    float adc_voltage = avg * conversion_factor;

    float r1 = 27000;
    float r2 = 4700;
    float i = adc_voltage / r2;
    float batt_voltage = i * (r1 + r2);

    return batt_voltage;
}

float CHwCheck::get_battery_voltage()
{
    // TODO: Not sure why this is off? Is this fiddle factor going to be different per unit, and so should be in EEPROM?
    return get_adc_voltage() + 1.22;
}

uint8_t CHwCheck::get_real_time_battery_percentage()
{
    /* Full  = 12.50v
       Empty = 10.50v (shutdown at this - still need to add h/w low voltage shutoff just below this)
     
       Limitations: When on charge, this will be >= 13v when almost full, but there's no definite way
                    to tell when a charger is plugged in
     */
    float batt_voltage = get_battery_voltage();
    float pc = ((batt_voltage - 10.5)/2) * 100;
    
    if (pc < 0)
        pc = 0;
    if (pc > 100)
        pc = 100;

    return (uint8_t)pc;
}

uint8_t CHwCheck::get_battery_percentage()
{
    // ignore 10 lowest values. get the average of the rest
    qsort(_batt_percentage, 10, sizeof(uint8_t), CHwCheck::cmpfunc_uint8_t);
    uint32_t total=0;
    for (uint8_t reading=10; reading < BAT_AVG_COUNT; reading++)
        total += _batt_percentage[reading];
    
    uint32_t avg = total/(BAT_AVG_COUNT-10);
    return avg;
}

void CHwCheck::process()
{
    // On initial startup all the readings will be 0, so try and set them to something better
    // as quick as possible.
    if (_inital_startup || (time_us_64() - _last_update > 1000000)) // 1sec
    {
        _batt_reading_idx++;
        if (_batt_reading_idx >= BAT_AVG_COUNT)
        {
            _batt_reading_idx = 0;
            _inital_startup = false;
        }

        _batt_percentage[_batt_reading_idx] = get_real_time_battery_percentage();

        _last_update = time_us_64();
    }
}
