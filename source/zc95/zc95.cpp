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

#include <stdio.h>
#include <vector>
#include <string.h>
#include <inttypes.h>

#include "globals.h"
#include "gDebugCounters.h"
#include "config.h"
#include "git_version.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "hardware/adc.h"

#include "CLedControl.h"
#include "Hal/IHal.h"
#include "Hal/HalMk1.h"
#include "Hal/HalMk2.h"
#include "Hal/HalDummy.h"

#include "CEeprom.h"
#include "CSavedSettings.h"
#include "CTimingTest.h"
#include "HwCheck/CHwCheck.h"
#include "HwCheck/CDetermineHardwareVersion.h"
#include "CAnalogueCapture.h"
#include "CDebugOutput.h"
#include "CRadio.h"
#include "Bluetooth/CBluetooth.h"

#include "AudioInput/CMCP4651.h"
#include "AudioInput/CAudio.h"

#include "display/CDisplay.h"
#include "display/CMainMenu.h"
#include "display/config/remote_access/CMenuApMode.h"

#include "core1/Core1.h"
#include "core1/Core1Messages.h"
#include "core1/output/CChannelConfig.h"
#include "core1/routines/CRoutines.h"
#include "core1/CRoutineOutput.h"
#include "core1/CRoutineOutputCore1.h"

#include "PowerManagement/CPowerManagementMk1.h"
#include "PowerManagement/CPowerManagementMk2.h"

#include "RemoteAccess/CWifi.h"
#include "RemoteAccess/CSerialConnection.h"


#include "ECButtons.h"
#include "FlashHelper.h"

IHal* _hal = NULL;
CEeprom eeprom = CEeprom(I2C_PORT, EEPROM_ADDR);
CAnalogueCapture _analogueCapture;
CMCP4651 audio_gain;
CAudio* _audio = NULL;
CWifi *wifi = NULL;
CRadio *radio = NULL;
extern CSerialConnection *g_SerialConnection;

void check_button(CMenu *current_menu, Button button)
{
    bool new_state = false;
    if (_hal->front_panel()->has_button_state_changed(button, &new_state))
    {
        if (new_state)
            current_menu->button_pressed(button);
        else
            current_menu->button_released(button);
    }
}

void process_front_panel_input(CMenu *current_menu)
{
    check_button(current_menu, Button::A);
    check_button(current_menu, Button::B);
    check_button(current_menu, Button::C);
    check_button(current_menu, Button::D);
    check_button(current_menu, Button::ROT);
}

void update_power_levels_from_front_panel(CRoutineOutput *routine_output)
{
    for (int chan=0; chan < MAX_CHANNELS; chan++)
    {
        uint16_t fp_power = _hal->front_panel()->get_channel_power_level(chan);
        routine_output->set_front_panel_power(chan, fp_power);   
    }
}

/* Stolen from https://www.raspberrypi.org/forums/viewtopic.php?t=302960
 * "Random number generator - an example" by bgolab Feb 2021 */
void seed_random_from_rosc()
{
  uint32_t random = 0x811c9dc5;
  uint8_t next_byte = 0;
  volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

  for (int i = 0; i < 16; i++) {
    for (int k = 0; k < 8; k++) {
      next_byte = (next_byte << 1) | (*rnd_reg & 1);
    }

    random ^= next_byte;
    random *= 0x01000193;
  }

  srand(random);
} 

void set_leds_to_black(CLedControl* led)
{
    led->set_all_led_colour(LedColour::Black);
    led->loop(true);
    sleep_ms(1);
    led->loop(true);
}

int main()
{
    CSavedSettings* settings = NULL;
    CLedControl led = CLedControl(PIN_LED, &settings);
    CRoutineOutput* routine_output  = NULL;
    set_leds_to_black(&led);

    // I2C Initialisation
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    mutex_init(&gI2cMutex);

    // Serial going to 3.5mm socket
    gpio_set_function(PIN_AUX_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_AUX_UART_RX, GPIO_FUNC_UART);
    
    // Serial on acc port
    gpio_set_function(PIN_ACC_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_ACC_UART_RX, GPIO_FUNC_UART);    
    
    // For now, until settings loaded from eeprom, send debugging info to accessory port
    CDebugOutput::set_debug_destination(CDebugOutput::debug_dest_t::ACC);
    printf("\n\nZC95 Startup, firmware version: %s\n", kGitHash);
    
    zc95_version_t hardware_version = CDetermineHardwareVersion::get_hardware_version();

    adc_init();
    messages_init();
    debug_counters_init();

    if (hardware_version == zc95_version_t::MKII)
    {
        printf("Hardware version: MKII\n");
        _hal = new HalMk2(&led, &routine_output, &_analogueCapture, &settings);
    }
    else if (hardware_version == zc95_version_t::MKI)
    {
        printf("Hardware version: MKI\n");
        _hal = new HalMk1(&led, &routine_output, &_analogueCapture, &settings);
    }
    else
    {
        printf("Hardware version: Unknown!\n");
        _hal = new HalDummy();
    }
    _hal->set_backlight(false);

    CHwCheck hw_check(_hal, &led);
    hw_check.check_part1(); // If a fault is found, this never returns

    _audio = new CAudio(&_analogueCapture, &audio_gain, _hal);
    _audio->set_audio_digipot_found(hw_check.audio_digipot_found());

    // Make sure there is some semi-random-ish data available
    seed_random_from_rosc();

    // Note eeprom ic is on i2c bus
    sleep_ms(100); // wait for eeprom to be ready
    settings = new CSavedSettings(&eeprom);
    g_SavedSettings = settings;

    // Configure AUX port for serial or audio use
    _hal->audio_input_enable(settings->get_aux_port_use() == CSavedSettings::setting_aux_port_use::AUDIO);

    CDebugOutput::set_debug_destination_from_settings(settings);

    // Front panel LEDs - give some feedback we're powering up (display takes almost second to appear)
    led.set_all_led_colour(LedColour::Purple);
    led.loop(true);
    sleep_ms(1);
    led.loop(true);

    radio = new CRadio(&_analogueCapture);
    CBluetooth bluetooth = CBluetooth(radio);

    // Configure SPI display
    CDisplay display = CDisplay(_hal->front_panel(), &bluetooth, _hal->power_management());
    display.init(); // This takes some time - not far off a second
    hw_check.set_display(&display);

    // Show the splash screen & start the update of the display via DMA. To avoid briefly
    // showing rubbish, wait for that inital update to complete, then turn on the backlight.
    display.show_splash_screen(); 
    sleep_ms(25);

    _hal->set_backlight(true);

    // Get list of available patterns / routines
    std::vector<CRoutines::Routine> routines;
    CRoutines::get_routines(routines);
   
    hw_check.check_part2(); // If a fault is found, this never returns

    // Queue used for pulses from audio processing on core0 being sent to core1 for output
    for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        queue_init(&gPulseQueue[channel], sizeof(pulse_message_t), PULSE_QUEUE_LENGTH);

    // Queue used for routines (so far just Lua) running on Core1 to send debug messages (via print())
    // out over a websocket connection, if running via RemoteAccess
    queue_init(&gPatternTextOutputQueue, sizeof(pattern_text_output_t), PATTERN_TEXT_OUTPUT_QUEUE_LENGTH);

    // Queue used to send bluetooth HID events to Lua scripts
    queue_init(&gBtRawHidQueue , sizeof(CBluetoothConnect::bt_raw_hid_queue_entry_t), 15);

    // Load/set gain, mic preamp, etc., from eeprom
    _audio->init(settings, &display);

    _analogueCapture.start();

    led.set_all_led_colour(LedColour::Black);

    sleep_ms(100);

    core1_start(routines, settings);
    routine_output = new CRoutineOutputCore1(&display, &led, _hal, _audio);

    _audio->set_routine_output(routine_output);
    wifi = new CWifi(radio, &_analogueCapture, routine_output, routines);
    flash_helper_init(&_analogueCapture, routine_output);

    CMainMenu routine_selection = CMainMenu(&display, routines, settings, routine_output, &hw_check, _audio, &_analogueCapture, wifi, &bluetooth, radio, _hal);
    routine_selection.show();
    CMenu *current_menu = &routine_selection;
    display.set_current_menu(current_menu);

    uint64_t start = time_us_64();
    led.loop();
    uint64_t last_analog_check = 0;
    _hal->loop();
    while (1) 
    {
        uint64_t loop_start = time_us_64();
        radio->loop();
        wifi->loop();

        display.update();
        process_front_panel_input(current_menu);

        int8_t adj = _hal->front_panel()->get_adjust_control_change();
        if (adj && current_menu)
        {
            current_menu->adjust_rotary_encoder_change(adj);
        }

        if (time_us_64() > last_analog_check + 50000)
        {
            _hal->front_panel()->process(true);
            last_analog_check = time_us_64();
        }
    
        update_power_levels_from_front_panel(routine_output);
        if (time_us_64() - start > 1000000) // every second
        {
            start = time_us_64();
            led.loop(true);
            uint64_t timenow = time_us_64();
            // printf("Loop time: %" PRId64 ", batt: %d\n", timenow - loop_start, batt_percentage);
        }
 
        routine_output->loop();
        led.loop();
        _analogueCapture.process();
        _audio->process();

        if (_audio->is_audio_update_available(true))
        {
            display.set_update_required();
        }

        _hal->loop();

        if (g_SerialConnection)
            g_SerialConnection->loop();

        if (gFatalError)
        {
            routine_output->stop_routine();
            hw_check.die(gErrorString); // never returns
        }
    }

    return 0;
}
