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
#include "config.h"
#include "CUtil.h"
#include "ECButtons.h"
#include "LuaScripts/LuaScripts.h"

/*
 * Check for the presence of all expected i2c devices. If any are missing, flash the LEDs and try to display an 
 * error on screen indicating what's missing. The backlight pin of the display is connected to the U7 port
 * expander, so if that's not working, the display might not be visible. 
 */
CHwCheck::CHwCheck(CBatteryGauge *batteryGauge)
{
    _zc624_comms = new CZC624Comms(ZC624_SPI_PORT, I2C_PORT);

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

void CHwCheck::set_expected_devices(CHwCheck::front_panel_version_t ver)
{
    _devices.clear();

    // These parts should always be present
    _devices.push_front(device(EXT_INPUT_PORT_EXP_ADDR, "Trigger+Acc port expander (U8)", "Port exp U8"));
    _devices.push_front(device(CONTROLS_PORT_EXP_ADDR, "Port expander for buttons (U7)", "Port exp U7"));
    _devices.push_front(device(EEPROM_ADDR, "EEPROM (read)", "EEPROM"));
    _devices.push_front(device(EEPROM_ADDR+1, "EEPROM (write)", "EEPROM"));
    _devices.push_front(device(ZC624_ADDR, "ZC624 output board", "ZC624"));
    
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
    _devices.push_front(device(AUDIO_DIGIPOT_ADDR, "Digital potentiometer on audio board", "Audio digipot", true));
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

    front_panel_version_t fp_version = determine_front_panel_version();
    set_expected_devices(fp_version);

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

    if (fp_version == front_panel_version_t::UNKNOWN)
    {
        cause = Cause::NO_FP_ADC;
        ok = false;
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

    clear_eeprom_if_requested(fp_version); // if appropriate button is held down, clears eeprom then halts

}

// The ZC624 output board takes a while to initialize from power on, so check its status much later when it should be ready.
// By the time this is called, the display and LEDs should be initialized, so need these passed in (CMainBoardPortExp controls display backlight)
void CHwCheck::check_part2(CLedControl *ledControl, CMainBoardPortExp *controls)
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
        hw_check_failed(cause, ledControl, controls); // this never returns
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

void CHwCheck::die(CLedControl *led_control, std::string error_message)
{
    int y = 0;
    hagl_clear(_hagl_backend);

    put_text("Fatal error", 0, (y++ * 10), hagl_color(_hagl_backend,0xFF, 0xFF, 0xFF));
    put_text("===========", 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));
    y++;
    put_text(error_message, 0, (y++ * 10), hagl_color(_hagl_backend, 0xFF, 0xFF, 0xFF));

    hagl_flush(_hagl_backend);

    halt(led_control);
}

void CHwCheck::hw_check_failed(enum Cause cause, CLedControl *ledControl, CMainBoardPortExp *controls)
{
    int y = 0;
    ledControl->set_all_led_colour(LedColour::Red);
    ledControl->loop();

    if (controls == NULL)
    {  
        _hagl_backend = hagl_init();
    }
    else
    {
        controls->set_lcd_backlight(true);
    }

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
        
        default:
            show_error_text_message(&y, "Unknown error");
            break;
    }

    hagl_flush(_hagl_backend);

    halt(ledControl);
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

void CHwCheck::put_text(std::string text, int16_t x, int16_t y, hagl_color_t color)
{
    if (text == "")
        text = " ";

    std::wstring widestr = std::wstring(text.begin(), text.end());
    hagl_put_text(_hagl_backend, widestr.c_str(), x, y, color, font6x9);
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
    CLedControl led = CLedControl(PIN_LED, NULL);
    led.init();
    led.set_all_led_colour(LedColour::Blue);
    led.loop();

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
            halt(&led);
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
            halt(&led);
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
        retval = i2c_read(__func__, CONTROLS_PORT_EXP_ADDR, &pin_states, 1, false);
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

void CHwCheck::process()
{

}

void CHwCheck::set_display(CDisplay *display)
{
    _hagl_backend = display->get_hagl_backed();
}

// Try to determine the front panel version.
// Identify v0.1 by the presence of the I/O expander on 0x26 (on v0.2 the I/O expander is on 0x38, but needs to be written to before it'll respond)
// Identify v0.2 by the presence of the ADC 0x49
CHwCheck::front_panel_version_t CHwCheck::determine_front_panel_version()
{
    uint8_t rx_data = 0;

    bool v0_1 = (i2c_read_timeout_us(i2c0, FP_0_1_PORT_EXP_ADDR, &rx_data, 1, false, 1000) > 0);
    bool v0_2 = (i2c_read_timeout_us(i2c0, FP_0_2_ADC_ADDR, &rx_data, 1, false, 1000) > 0);

    if (v0_1 && v0_2)
    {
        printf("ERROR: unable to determine front panel version: device found at address 0x%x (v0.1 IO expander) AND address 0x%x (v0.2 ADC) ?!\n", 
            FP_0_1_PORT_EXP_ADDR, FP_0_2_ADC_ADDR);
        printf("       (only one - either - expected)\n");
        _front_panel_version = front_panel_version_t::UNKNOWN;
    }
    else if (v0_1)
    {
        printf("Front panel version 0.1\n");
        _front_panel_version = front_panel_version_t::v0_1;
    }
    else if (v0_2)
    {
        printf("Front panel version 0.2\n");
        init_v0_2_front_panel();
        _front_panel_version = front_panel_version_t::v0_2;
    }
    else
    {
        printf("ERROR: unable to determine front panel version: No device found on either address 0x%x (v0.1 IO expander) or address 0x%x (v0.2 ADC)\n",
             FP_0_1_PORT_EXP_ADDR, FP_0_2_ADC_ADDR);
        _front_panel_version = front_panel_version_t::UNKNOWN;
    }

    return _front_panel_version;
}

// v0.2 of the front panel has the reset line of the LED driver connected to the I/O expander
// without a pullup (rookie error!). So to get the LED driver out of reset (and responding on
// the i2c bus), we need to get the I/O expander to set that high. 
// The LED driver, by default, also responds on the i2c address 0x48 as well as the configured
// one, so disable that feature too.
// This all wants doing now, in the H/W check, so the next step finds the LED driver, and if we 
// restart without a power cycle (i.e. via debugger), the LED driver also showing up on 0x48 
// doesn't get confused with the ADC in v0.1 of the front panel, which is on 0x48 too :(
void CHwCheck::init_v0_2_front_panel()
{
    uint8_t txbuf[2] = {0};
    txbuf[0] = 0x03;    // Configuration register
    txbuf[1] = ~(0x10); // Set p4 (only) to output

    i2c_write_timeout_us(i2c0, FP_0_2_PORT_EXP_ADDR, txbuf , 2, false , 1000);

    // As the reset line was floating, pull it low before high, just to make sure it's correctly reset.

    // Set p4 to low
    txbuf[0] = 0x01;  // Output port register
    txbuf[1] = 0x00;  // Set all low
    i2c_write_timeout_us(i2c0, FP_0_2_PORT_EXP_ADDR, txbuf, 2, false, 1000);
    sleep_ms(1);

    // Set high (exit reset)
    txbuf[1] = 0x10;  // Set p4 (only) to high
    i2c_write_timeout_us(i2c0, FP_0_2_PORT_EXP_ADDR, txbuf, 2, false, 1000);
    sleep_ms(1);

    // Init LED driver. Don't respond on the ALLCALL address of "90h". Which is really 
    // the 7bit address 0x48 - the same as the ADC on v0.1 of the front panel...
    txbuf[0] = 0x00;  // MODE1
    txbuf[1] = 0x00; 
    if (i2c_write_timeout_us(i2c0, FP_0_2_BUTTON_LED_DRV_ADDR, txbuf, 2, false, 1000) < 0)
    {
        printf("init_v0_2_front_panel: i2c write error\n");
    }
}

CHwCheck::front_panel_version_t CHwCheck::get_front_panel_version()
{
    return _front_panel_version;
}
