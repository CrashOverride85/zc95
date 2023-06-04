/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
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
#include "CUtil.h"
#include "ECButtons.h"

/*
 * Check for the presence of all expected i2c devices. If any are missing, flash the LEDs and try to display an 
 * error on screen indicating what's missing. The backlight pin of the display is connected to the U7 port
 * expander, so if that's not working, the display might not be visible. 
 */
CHwCheck::CHwCheck(CBatteryGauge *batteryGauge)
{
    _zc624_comms = new CZC624Comms(NULL, I2C_PORT);

    _devices.push_front(device(EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U8)", "Port exp U8"));
    _devices.push_front(device(CONTROLS_PORT_EXP_ADDR, "Port expander for buttons (U7)", "Port exp U7"));
    _devices.push_front(device(EEPROM_ADDR, "EEPROM (read)", "EEPROM"));
    _devices.push_front(device(EEPROM_ADDR+1, "EEPROM (write)", "EEPROM"));
    _devices.push_front(device(ZC624_ADDR, "ZC624 output board", "ZC624"));
    
    // these two ICs are on the front panel
    _devices.push_front(device(ADC_ADDR, "Front panel ADC", "FP ADC U1"));
    _devices.push_front(device(FP_ANALOG_PORT_EXP_2_ADDR, "Front panel port expander (U2)", "FP Port Exp U2"));

    // optional parts
    _devices.push_front(device(AUDIO_DIGIPOT_ADDR, "Digital potentiometer on audio board", "Audio digipot", true));

    _batteryGauge = batteryGauge;
}

CHwCheck::~CHwCheck()
{
    if (_zc624_comms)
    {
        delete _zc624_comms;
        _zc624_comms = NULL;
    }
}

void CHwCheck::check_part1()
{
    bool ok = true;
    int ret;
    uint8_t rxdata;

    enum Cause cause = Cause::UNKNOWN;

    printf("\n\nHardware check (part1)\n");
    printf("======================\n");

    running_on_picow();

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

    clear_eeprom_if_requested(); // if appropriate button is held down, clears eeprom then halts

}

// The ZC624 output board takes a while to initialize from power on, so check its status much later when it should be ready.
// By the time this is called, the display and LEDs should be initialized, so need these passed in (CControlsPortExp controls display backlight)
void CHwCheck::check_part2(CLedControl *ledControl, CControlsPortExp *controls)
{
    uint8_t ver_minor = 0;
    uint8_t ver_major = 0;
    bool ver_check_ok = _zc624_comms->get_major_minor_version(&ver_major, &ver_minor);
    CHwCheck::Cause cause = Cause::ZC624_UNKNOWN;
    bool error = false;

    printf("\n\nHardware check (part2)\n");
    printf("======================\n");

    printf("    ZC624 Version...");
    if (ver_check_ok)
    {
        printf("API maj=[%d], min=[%d], FW=[%s]\n", ver_major, ver_minor, _zc624_comms->get_version().c_str());

        // check version is compatable
        if (ZC624_REQUIRED_MAJOR_VERION != ver_major || ver_minor < ZC624_MIN_MINOR_VERION)
        {
            printf("ZC624 API version mismatch. Expected:\n");
            printf("  major version  = %d (found %d)\n", ZC624_REQUIRED_MAJOR_VERION, ver_major);
            printf("  minor version >= %d (found %d)\n", ZC624_MIN_MINOR_VERION     , ver_minor);
            error = true;
            cause = Cause::ZC624_VERSION;
        }
    }
    else
    {
        printf("ERROR\n");
        error = true;
    }

    if (!error)
    {
        printf("    ZC624 status...");
        if (!(_zc624_comms->check_zc624()))
        {
            printf("FAULT\n");
            error = true;
            cause = Cause::ZC624_STATUS;
        }
    }

    if (error)
    {
        hw_check_failed(cause, ledControl, controls); // this never returns
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

void CHwCheck::die(CLedControl *led_control, std::string error_message)
{
    int y = 0;
    hagl_clear_screen();

    put_text("Fatal error", 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));
    put_text("===========", 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));
    y++;
    put_text(error_message, 0, (y++ * 10), hagl_color(0xFF, 0xFF, 0xFF));

    hagl_flush();

    halt(led_control);
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

        case Cause::ZC624_STATUS:
            show_error_text_message(y, "ZC624 (output) fault");
            break;

        case Cause::ZC624_VERSION:
            show_error_text_message(y, "ZC624 version mismatch");
            break;

        case Cause::ZC624_UNKNOWN:
            show_error_text_message(y, "Unknown ZC624 error");
            break;
        
        default:
            show_error_text_message(y, "Unknown error");
            break;
    }

    hagl_flush();

    halt(ledControl);
}

// Try and determine if running on a Pico or Pico W, based on code from 
// "connecting-to-the-internet-with-pico-w.pdf", section 2.4 (Raspberry 
// Pi Ltd datasheet)
// Should be called for the first time right at startup - definitely before
// analog capture has started.
bool CHwCheck::running_on_picow()
{
    static bool already_ran = false;
    static bool on_pi_w = false;

    if (already_ran)
    {
        return on_pi_w;
    }
    already_ran = true;

    adc_gpio_init(29);

    adc_select_input(3);
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;
    // printf("ADC3 value: 0x%03x, voltage: %f V\n", result, voltage);

    if (voltage < 0.3)
    {
        printf("Running on Pico W (probably: ADC3 voltage = %fv)\n", voltage);
        on_pi_w = true;
    }
    else
    {
        printf("Running on Pico non-W (probably: ADC3 voltage = %fv)\n", voltage);
        on_pi_w = false;
    }

    return on_pi_w;
}

void CHwCheck::halt(CLedControl *led_control)
{
    printf("Core0: HALT.\n");
    while(1)
    {
        sleep_ms(1000);
        led_control->set_all_led_colour(LedColour::Black);
        led_control->loop();
        
        sleep_ms(1000);
        led_control->set_all_led_colour(LedColour::Red);
        led_control->loop();
    };
}

std::string CHwCheck::get_zc624_version()
{
    return _zc624_comms->get_version();
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

// Returns true if the top right button (C) is pressed, and the bottom left button (B)
// is NOT pressed. This in case there's some fault causing all buttons to be read as pressed,
// we don't want to clear the EEPROM by mistake.
// Should only be ran after inital h/w check has confirmed the port expander 
// is present.
bool CHwCheck::clear_eeprom_buttons_pressed()
{
    uint8_t pin_states = 0;
    int retval = i2c_read(__func__, CONTROLS_PORT_EXP_ADDR, &pin_states, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CHwCheck::clear_eeprom_buttons_pressed i2c read error!\n");
      pin_states = 0;
    }

    bool button_pressed = (pin_states & (1 << (uint8_t)Button::C)) && 
                         !(pin_states & (1 << (uint8_t)Button::B));

    printf("CHwCheck::clear_eeprom_buttons_pressed(): button pressed = %d (pin state = %x)\n", button_pressed, pin_states);
    return button_pressed;
}

// If the top right button is pressed, show a basic screen asking if the eeprom should be cleared.
// To keep things simple (no need to worry about blocking, partially initialised stuff afterwards, 
// etc.), once the confirmation screen is displayed, the only way out is a power cycle.
void CHwCheck::clear_eeprom_if_requested()
{
    if (!clear_eeprom_buttons_pressed())
        return;

    // Ok, button is held down indicating eeprom should be cleared. Show confirmation screen.
    // From this point on, the only way out is a power-cycle - either with or without
    // clearing eeprom first
    CLedControl led = CLedControl(PIN_LED, NULL);
    led.init();
    led.set_all_led_colour(LedColour::Blue);
    led.loop();

    hagl_init();
    put_text("Clear EEPROM?", 0, 0, hagl_color(0xFF, 0xFF, 0xFF));
    put_text("Yes", 0, (DISPLAY_HEIGHT-1) - 10, hagl_color(0xAA, 0xAA, 0xAA));
    hagl_flush();

    // Wait for bottom right (B) button to be pressed for ~100ms
    while (1)
    {
        uint8_t pin_states = 0;
        i2c_read(__func__, CONTROLS_PORT_EXP_ADDR, &pin_states, 1, false);

        bool button_pressed = (pin_states & (1 << (uint8_t)Button::B));
        if (button_pressed)
        {
            // Do crude debounce; re-read pin after 100ms and if button is still pressed reset EEPROM
            sleep_ms(100);

            i2c_read(__func__, CONTROLS_PORT_EXP_ADDR, &pin_states, 1, false);
            button_pressed = (pin_states & (1 << (uint8_t)Button::B));

            if (button_pressed)
            {
                // Time to reset the EEPROM!
                printf("Clearing EEPROM at user request\n");

                CEeprom eeprom = CEeprom(I2C_PORT, EEPROM_ADDR);
                CSavedSettings settings = CSavedSettings(&eeprom);
                settings.eeprom_initialise();

                hagl_clear_screen();
                put_text("EEPROM cleared!", 0, 0, hagl_color(0xFF, 0xFF, 0xFF));
                hagl_flush();
                halt(&led);
            }
        }
    }
}

void CHwCheck::process()
{

}
