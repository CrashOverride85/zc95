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
CHwCheck::CHwCheck(CBatteryGauge *batteryGauge)
{
    _devices.push_front(device(EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U8)", "Port exp U8"));
    _devices.push_front(device(CONTROLS_PORT_EXP_ADDR, "Port expander for buttons (U7)", "Port exp U7"));
    _devices.push_front(device(EEPROM_ADDR, "EEPROM (read)", "EEPROM"));
    _devices.push_front(device(EEPROM_ADDR+1, "EEPROM (write)", "EEPROM"));
    _devices.push_front(device(ZC624_ADDR, "ZC624 output board", "ZC624"));
    
    // these two ICs are on the front panel
    _devices.push_front(device(ADC_ADDR, "Front pannel ADC", "FP ADC U1"));
    _devices.push_front(device(FP_ANALOG_PORT_EXP_2_ADDR, "Front pannel port expander (U2)", "FP Port Exp U2"));

    // optional parts
    _devices.push_front(device(AUDIO_DIGIPOT_ADDR, "Digital potentiometer on audio board", "Audio digipot", true));

    _batteryGauge = batteryGauge;
}

void CHwCheck::check_part1()
{
    bool ok = true;
    int ret;
    uint8_t rxdata;

    enum Cause cause = Cause::UNKNOWN;

    printf("\n\nHardware check (part1)\n");
    printf("======================\n");
    
    for (uint x=0; x < 10; x++)
        get_battery_readings();

    // Check battery isn't flat
    uint8_t batt_percentage = _batteryGauge->get_battery_percentage();
    printf("Battery: %d%%\n", batt_percentage);
    if (batt_percentage == 0)
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
            if (it->optional)
            {
                printf("Not found\n");
            }
            else
            {
                printf("NOT FOUND! (expected on address %d)\n", it->address);
                cause = Cause::MISSING;
                ok = false;
            }
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

bool CHwCheck::audio_digipot_found()
{
    for (std::list<device>::iterator it = _devices.begin(); it != _devices.end(); ++it)
        if (it->address == AUDIO_DIGIPOT_ADDR)
            return it->present;

    printf("CHwCheck::audio_digipot_found(): Unable to determine if digipot present\n");
    return false;
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
        if (!it->present && !it->optional)
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

void CHwCheck::get_battery_readings()
{
    uint8_t readings[10];

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    // get 10 readings
    for (uint8_t reading_count=0; reading_count < sizeof(readings); reading_count++)
    {
        uint16_t reading = adc_read();
        readings[reading_count] = reading >> 4; // Convert 12bit ADC reading to 8bit (later reads via DMA are 8bit, so need to be consistant)
    }

    _batteryGauge->add_raw_adc_readings(readings, sizeof(readings));
}

void CHwCheck::process()
{

}
