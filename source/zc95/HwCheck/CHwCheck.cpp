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
#include "hardware/flash.h"
#include "pico/btstack_flash_bank.h"

#include <font6x9.h>
#include "CHwCheck.h"
#include "i2c_scan.h"
#include "../config.h"
#include "../CUtil.h"
#include "../ECButtons.h"
#include "../LuaScripts/LuaScripts.h"

/*
 * Check for the presence of all expected i2c devices. If any are missing, flash the LEDs and try to display an 
 * error on screen indicating what's missing. The backlight pin of the display is connected to the U7 port
 * expander, so if that's not working, the display might not be visible. 
 */
CHwCheck::CHwCheck(IHal* hal, CLedControl *led)
{
    _hal = hal;
    _led = led;
    _zc624_comms = new CZC624Comms(ZC624_SPI_PORT, I2C_PORT);
}

CHwCheck::~CHwCheck()
{
    if (_zc624_comms)
    {
        delete _zc624_comms;
        _zc624_comms = NULL;
    }
}

void CHwCheck::set_expected_devices(front_panel_version_t ver, zc95_version_t hw_ver)
{
    _devices.clear();

    // These parts should always be present
    _devices.push_front(device(EEPROM_ADDR, "EEPROM (read)", "EEPROM"));
    _devices.push_front(device(EEPROM_ADDR+1, "EEPROM (write)", "EEPROM"));
    _devices.push_front(device(ZC624_ADDR, "ZC624 output board", "ZC624"));

    // The MkI uses a different type of port expander on different addresses to the MKII
    if (hw_ver == zc95_version_t::MKI)
    {
        _devices.push_front(device(MK1_EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U8)", "Port exp U8"));
        _devices.push_front(device(MK1_CONTROLS_PORT_EXP_ADDR, "Port expander for buttons (U7)", "Port exp U7"));
    }
    else
    {
        _devices.push_front(device(MK2_EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U1)", "Port exp U1"));
        _devices.push_front(device(MK2_PORT_EXP_ADDR, "Port expander for charger & audio (U28)", "Port exp U28"));
    }
    
    // There ICs are on the front panel, but which set depends on the front panel version
    // v0.1
    if (ver == front_panel_version_t::v0_1)
    {
        _devices.push_front(device(FP_0_1_ADC_ADDR, "Front panel (v0.1) ADC", "FP ADC U1"));
        _devices.push_front(device(FP_0_1_PORT_EXP_ADDR, "Front panel (v0.1) port expander (U2)", "FP Port Exp U2"));
    }

    // >= v0.2
    if (ver == front_panel_version_t::v0_2)
    {
        _devices.push_front(device(FP_0_2_ADC_ADDR, "Front panel (v0.2) ADC", "FP ADC U1"));
        _devices.push_front(device(FP_0_2_PORT_EXP_ADDR, "Front panel (v0.2) port expander (U2)", "FP Port Exp U2"));
        _devices.push_front(device(FP_0_2_BUTTON_LED_DRV_ADDR, "Front panel (v0.2) LED driver (U9)", "FP LED drv U9"));
    }

    // optional parts
    _devices.push_front(device(AUDIO_DIGIPOT_ADDR , "Digital potentiometer on audio board", "Audio digipot", true));
    _devices.push_front(device(BQ72441_I2C_ADDRESS, "BQ72441 Fuel/gas gauge", "Fuel gauge", true));
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

    printf("I2C scan:\n");
    i2c_scan::scan(i2c0);
    printf("\n");

    // Check battery isn't flat
    if (_hal->hardware_version() == zc95_version_t::MKI) // Mk2 has hardware low voltage cutoff
    {
        uint8_t batt_percentage = _hal->power_management()->get_battery_percentage();
        printf("Battery: %d%%\n", batt_percentage);
        if (batt_percentage == 0)
        {
            printf("Battery is flat!\n");
            ok = false;
            cause = Cause::BATTERY;
        }
    }

    set_expected_devices(_hal->front_panel()->verion(), _hal->hardware_version());

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
                printf("NOT FOUND! (expected on address 0x%x)\n", it->address);
                cause = Cause::MISSING;
                ok = false;
            }
        }
    }

    if (_hal->front_panel()->verion() == front_panel_version_t::UNKNOWN)
    {
        cause = Cause::NO_FP_ADC;
        ok = false;
    }

    if (_hal->hardware_version() == zc95_version_t::UNKNOWN)
    {
        cause = Cause::HW_VER_UNKNOWN;
        ok = false;
    }

    if (ok)
    {
        printf("Status: Ok\n\n");
    }
    else
    {
        printf("Status: FAILED\n\n");
        hw_check_failed(cause); // this never returns
    }

    clear_eeprom_if_requested(_hal->front_panel()->verion()); // if appropriate button is held down, clears eeprom then halts

}

// The ZC624 output board takes a while to initialize from power on, so check its status much later when it should be ready.
// By the time this is called, the display and LEDs should be initialized, so need these passed in (CMainBoardPortExp controls display backlight)
void CHwCheck::check_part2()
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
        _zc624_status = _zc624_comms->check_zc624();
        if (_zc624_status)
        {
            printf("FAULT\n");
            error = true;
            cause = Cause::ZC624_STATUS;
        }
        else
        {
            printf("Ok\n");
        }
    }

    if (!error)
    {
        printf("    ZC624 SPI comms...");
        if (_zc624_comms->spi_has_comms_fault())
        {
            printf("FAULT\n");
            error = true;
            cause = Cause::ZC624_NO_SPI;
        }
        else
        {
            printf("Ok\n");
        }
    }

    printf("\n");
    if (error)
    {
        hw_check_failed(cause); // this never returns
    }
}

bool CHwCheck::audio_digipot_found()
{
    for (std::list<device>::iterator it = _devices.begin(); it != _devices.end(); ++it)
        if (it->address == AUDIO_DIGIPOT_ADDR)
            return it->present;

    printf("CHwCheck::audio_digipot_found(): Unable to determine if digipot present\n");
    return false;
}

void CHwCheck::show_error_text_message(int *y, std::string message)
{
    put_text(message, 0, ((*y)++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
}

void CHwCheck::show_error_text_missing(int y)
{
    y += 2;
    put_text("Missing:", 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
    
    for (std::list<device>::iterator it = _devices.begin(); it != _devices.end(); ++it)
    {
        if (!it->present && !it->optional)
        {
            put_text("   * " + it->display + "\n", 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
        }
    }    
}

void CHwCheck::die(std::string error_message)
{
    int y = 0;
    hagl_clear(_hagl_backend);

    put_text("Fatal error", 0, (y++ * 10), hagl_color(_hagl_backend,0xFF, 0xFF, 0xFF));
    put_text("===========", 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
    y++;
    put_text(error_message, 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));

    hagl_flush(_hagl_backend);

    halt();
}

void CHwCheck::hw_check_failed(enum Cause cause)
{
    int y = 0;
    _led->set_all_led_colour(LedColour::Red);
    _led->loop();

    if (_hagl_backend == NULL)
    {  
        _hagl_backend = hagl_init();
    }

    _hal->set_backlight(true);
    

    hagl_clear(_hagl_backend);

    put_text("Hardware check failed", (y++ * 10), 10, hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));

    y += 2;

    switch (cause)
    {
        case Cause::MISSING:
            show_error_text_missing(y);
            break;

        case Cause::BATTERY:
            show_error_text_message(&y, "Battery is flat!");
            break;

        case Cause::ZC624_STATUS:
            report_zc624_fault(&y);
            break;

        case Cause::ZC624_VERSION:
            show_error_text_message(&y, "ZC624 version mismatch");
            break;

        case Cause::ZC624_UNKNOWN:
            show_error_text_message(&y, "Unknown ZC624 error");
            break;

        case Cause::ZC624_NO_SPI:
            show_error_text_message(&y, "SPI comms error with ZC624");
            break;

        case Cause::NO_FP_ADC:
            // With no ADC found, can't tell which version of the FP is connected, so don't know what other devices to look for
            show_error_text_message(&y, "Unable to determine");
            show_error_text_message(&y, "front panel version");
            break;

        case Cause::HW_VER_UNKNOWN:
            // Not been able to figure out if running on a MKI or MKII
            show_error_text_message(&y, "Unable to determine");
            show_error_text_message(&y, "hardware version   ");
            break;

        default:
            show_error_text_message(&y, "Unknown error");
            break;
    }

    hagl_flush(_hagl_backend);

    halt();
}

void CHwCheck::report_zc624_fault(int *y)
{
    std::string chan_state;
    show_error_text_message(y, "ZC624 (output) fault");

    // If the status is 0xFF, it means we couldn't read the status, so don't output what we don't know
    if (_zc624_status != 0xFF)
    {
        put_text("Overall status: FAULT", 0, ((*y)++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
        for (uint8_t chan = 1; chan <= MAX_CHANNELS; chan++)
        {
            if (_zc624_status & 1 << chan)
                chan_state = "FAULT";
            else
                chan_state = "OK";

            put_text("Channel " + std::to_string(chan) + "     : " + chan_state, 0, ((*y)++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
        }
    }
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

void CHwCheck::halt()
{
    printf("Core0: HALT.\n");
    while(1)
    {
        sleep_ms(1000);
        _led->set_all_led_colour(LedColour::Black);
        _led->loop();
        
        sleep_ms(1000);
        _led->set_all_led_colour(LedColour::Red);
        _led->loop();
    };
}

std::string CHwCheck::get_zc624_version()
{
    return _zc624_comms->get_version();
}

void CHwCheck::put_text(std::string text, int16_t x, int16_t y, hagl_color_t color)
{
    if (text == "")
        text = " ";

    std::wstring widestr = std::wstring(text.begin(), text.end());
    hagl_put_text(_hagl_backend, widestr.c_str(), x, y, color, font6x9);
}

// Returns true if (only) the top right button (C) is pressed.
// This in case there's some fault causing all buttons to be read as pressed,
// we don't want to clear the EEPROM by mistake.
// Should only be ran after inital h/w check has confirmed the port expander 
// is present.
bool CHwCheck::clear_eeprom_buttons_pressed(front_panel_version_t fp_version)
{
    uint8_t button_states = 0;
    button_states = get_button_states_from_port_expander(fp_version);

    bool button_pressed = (button_states & (1 << (uint8_t)Button::C)) && 
                         !(button_states & (1 << (uint8_t)Button::A)) && 
                         !(button_states & (1 << (uint8_t)Button::B)) && 
                         !(button_states & (1 << (uint8_t)Button::D));

    printf("CHwCheck::clear_eeprom_buttons_pressed(): button pressed = %d (pin state = 0x%X)\n", button_pressed, button_states);
    return button_pressed;
}

// If the top right button is pressed, show a basic screen asking if the eeprom should be cleared.
// To keep things simple (no need to worry about blocking, partially initialised stuff afterwards, 
// etc.), once the confirmation screen is displayed, the only way out is a power cycle.
void CHwCheck::clear_eeprom_if_requested(front_panel_version_t fp_version)
{
    if (!clear_eeprom_buttons_pressed(fp_version))
        return;

    // Ok, button is held down indicating eeprom should be cleared. Show confirmation screen.
    // From this point on, the only way out is a power-cycle - either with or without
    // clearing eeprom first
    _led->set_all_led_colour(LedColour::Blue);
    _led->loop();

    _hal->set_backlight(true);

    _hagl_backend = hagl_init();
    put_text("Clear saved settings?", 0, 0, hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
    put_text("EEPROM", 0                    , (MIPI_DISPLAY_HEIGHT-1) - 10, hagl_color(_hagl_backend, 0xAA, 0xAA, 0xAA));
    put_text("Flash" , MIPI_DISPLAY_WIDTH-40, (MIPI_DISPLAY_HEIGHT-1) - 10, hagl_color(_hagl_backend, 0xAA, 0xAA, 0xAA));
    hagl_flush(_hagl_backend);

    // Wait for bottom left (B) or bottom right (D) button to be pressed for ~100ms
    while (1)
    {
        int button_pressed = get_button_press(fp_version);
        
        if (button_pressed == (int)Button::B) // clear EEPROM
        {
            // Time to reset the EEPROM!
            printf("Clearing EEPROM at user request\n");

            CEeprom eeprom = CEeprom(I2C_PORT, EEPROM_ADDR);
            CSavedSettings settings = CSavedSettings(&eeprom);
            settings.eeprom_initialise();

            hagl_clear(_hagl_backend);
            put_text("EEPROM cleared!", 0, 0, hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
            hagl_flush(_hagl_backend);
            halt();
        }
        else if (button_pressed == (int)Button::D) // Clear user settings in Flash
        {
            printf("Clearing saved settings in flash at user request\n");
            printf("Clear btstack config\n");
            flash_range_erase(PICO_FLASH_BANK_STORAGE_OFFSET, PICO_FLASH_BANK_TOTAL_SIZE);

            printf("Clear Lua scripts\n");
            // Loop through and add all valid lua scripts
            for (uint8_t index = 0; index < lua_script_count(); index++)
            {
                if (lua_scripts[index].writeable)
                {
                    printf("Erasing script slot [%d] (%lu to %lu)\n", index, lua_scripts[index].start, lua_scripts[index].end);
                    flash_range_erase(lua_scripts[index].start - XIP_BASE, lua_scripts[index].end - lua_scripts[index].start);
                }   
            }

            hagl_clear(_hagl_backend);
            put_text("User settings in", 0, 0, hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
            put_text("flash cleared!"  , 0, 8, hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
            hagl_flush(_hagl_backend);
            halt();
        }
    }
}

// Returns Button::B if B pushed, Button::D if D pushed, -1 otherwise.
int CHwCheck::get_button_press(front_panel_version_t fp_version)
{
        uint8_t button_states = get_button_states_from_port_expander(fp_version);

        bool button_pressed = (button_states & (1 << (uint8_t)Button::B)) || 
                              (button_states & (1 << (uint8_t)Button::D));
        if (button_pressed)
        {
            // Do crude debounce; re-read pin after 100ms and if button is still pressed reset EEPROM
            sleep_ms(100);

            button_states = get_button_states_from_port_expander(fp_version);
            button_pressed = (button_states & (1 << (uint8_t)Button::B)) || 
                             (button_states & (1 << (uint8_t)Button::D));

            if (button_pressed)
            {
                if (button_states & (1 << (uint8_t)Button::B))
                    return (int)Button::B;
                else if (button_states & (1 << (uint8_t)Button::D))
                    return (int)Button::D;
                else 
                    return -1;
            }
        }

    return -1;
}

uint8_t CHwCheck::get_button_states_from_port_expander(front_panel_version_t fp_version)
{
    uint8_t retval = 0;

    // For both v0.1 and v0.2 front panels, the buttons A->D are attached to I/O pins 0->3

    if (fp_version == front_panel_version_t::v0_1)
    {
        uint8_t pin_states = 0;
        retval = i2c_read(__func__, MK1_CONTROLS_PORT_EXP_ADDR, &pin_states, 1, false);
        if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
        {
            printf("CHwCheck::get_button_states_from_port_expander i2c read error!\n");
            pin_states = 0;
        }

        return pin_states & 0xF;
    }
    
    else if (fp_version == front_panel_version_t::v0_2)
    {
        uint8_t buffer[1] = {0};
        
        buffer[0] = 0; // port_exp_reg_t::INPUT_PORT;
        i2c_write(__func__, FP_0_2_PORT_EXP_ADDR, buffer, 1, false);
        retval = i2c_read (__func__, FP_0_2_PORT_EXP_ADDR, buffer, 1, false);

        if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
        {
            printf("CHwCheck::get_button_states_from_port_expander i2c read error!\n");
            buffer[0] = 0xFF;
        }

        return (~buffer[0]) & 0xF; // bit is low if button pushed, high if not pushed. Opposite to v0.1 front panel
    }
    
    else
    {
        printf("CHwCheck::get_button_states_from_port_expander(): ERROR - unexpected fp_version: [%d]\n", (uint8_t)fp_version);
        return 0;
    }
}

void CHwCheck::set_display(CDisplay *display)
{
    _hagl_backend = display->get_hagl_backed();
}
