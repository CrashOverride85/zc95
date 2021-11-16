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
#include "CLedControl.h"

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

    // these two ICs are on the front panel
    _devices.push_front(device(ADC_ADDR, "Front pannel ADC", "FP ADC U1"));
    _devices.push_front(device(FP_ANALOG_PORT_EXP_2_ADDR, "Front pannel port expander (U2)", "FP Port Exp U2"));
}

void CHwCheck::check()
{
    bool ok = true;
    int ret;
    uint8_t rxdata;

    enum Cause casue = Cause::UNKNOWN;

    printf("\n\nHardware check\n");
    printf("==============\n\n");
    
    // Check battery isn't flat
    if (get_battery_percentage() == 0)
    {
        printf("Battery is flat!\n");
        ok = false;
        casue = Cause::BATTERY;
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
            casue = Cause::MISSING;
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
        hw_check_failed(casue);
    }
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

void CHwCheck::hw_check_failed(enum Cause casue)
{
    int y = 0;
    CLedControl led = CLedControl(PIN_LED, NULL);
    led.init();
    led.set_all_led_colour(LedColour::Red);
    led.loop();

    hagl_init();
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
        
        default:
            show_error_text_message(y, "Unknown error");
            break;
    }

    hagl_flush();

    while(1)
    {
        sleep_ms(1000);
        led.set_all_led_colour(LedColour::Black);
        led.loop();
        
        sleep_ms(1000);
        led.set_all_led_colour(LedColour::Red);
        led.loop();
    };
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

float CHwCheck::get_battery_voltage(float adc_voltage)
{
    // TODO: Not sure why this is off? Is this fiddle factor going to be different per unit, and so should be in EEPROM?
    return get_adc_voltage() + 1.22;
}

uint8_t CHwCheck::get_battery_percentage()
{
    /* Full  = 12.50v
       Empty = 10.50v (shutdown at this - still need to add h/w low voltage shutoff just below this)
     
       Limitations: When on charge, this will be >= 13v when almost full, but there's no definite way
                    to tell when a charger is plugged in
     */
    float batt_voltage = get_battery_voltage(get_adc_voltage());
    float pc = ((batt_voltage - 10.5)/2) * 100;
    
    if (pc < 0)
        pc = 0;
    if (pc > 100)
        pc = 100;

    return (uint8_t)pc;
}
