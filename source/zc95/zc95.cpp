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

#include "i2c_scan.h"
#include "CLedControl.h"
#include "CMainBoardPortExp.h"
#include "CExtInputPortExp.h"
#include "CEeprom.h"
#include "CSavedSettings.h"
#include "CTimingTest.h"
#include "CHwCheck.h"
#include "CAnalogueCapture.h"
#include "CBatteryGauge.h"
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

#include "RemoteAccess/CWifi.h"
#include "RemoteAccess/CSerialConnection.h"


#include "FrontPanel/CFrontPanelV01.h"
#include "FrontPanel/CFrontPanelV02.h"
#include "ECButtons.h"
#include "FlashHelper.h"

CMainBoardPortExp port_expander = CMainBoardPortExp(CONTROLS_PORT_EXP_ADDR);
CExtInputPortExp *ext_input = NULL;
CEeprom eeprom = CEeprom(I2C_PORT, EEPROM_ADDR);
CFrontPanel *_front_panel = NULL;
CAnalogueCapture analogueCapture;
CBatteryGauge batteryGauge;
CMCP4651 audio_gain;
CAudio audio(&analogueCapture, &audio_gain, &port_expander);
CWifi *wifi = NULL;
CRadio *radio = NULL;
extern CSerialConnection *g_SerialConnection;

void gpio_callback(uint gpio, uint32_t events) 
{
    if (gpio == PIN_CONTROLS_INT)
    {
        port_expander.interrupt();
    }
    else if (gpio == PIN_EXT_INPUT_INT)
    {
        if (ext_input)
            ext_input->interrupt();
    }
    else if (gpio == PIN_FP_INT1)
    {
        if (_front_panel != NULL)
            _front_panel->interrupt(CFrontPanel::interrupt_t::INT1);
    }
    else if (gpio == PIN_FP_INT2)
    {
        if (_front_panel != NULL)
            _front_panel->interrupt(CFrontPanel::interrupt_t::INT2);
    }
}

void check_button(CMenu *current_menu, Button button)
{
    bool new_state = false;
    if (_front_panel->has_button_state_changed(button, &new_state))
    {
        if (new_state)
            current_menu->button_pressed(button);
        else
            current_menu->button_released(button);
    }
}

void process_front_panel_input(CMainBoardPortExp *port_expander, CMenu *current_menu)
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
        uint16_t fp_power = _front_panel->get_channel_power_level(chan);
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

int main()
{
    // Serial going to 3.5mm aux socket
    gpio_set_function(PIN_AUX_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_AUX_UART_RX, GPIO_FUNC_UART);
    
    // Serial on acc port
    gpio_set_function(PIN_ACC_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_ACC_UART_RX, GPIO_FUNC_UART);    
    
    // For now, until settings loaded from eeprom, send debugging info to accessory port
    CDebugOutput::set_debug_destination(CDebugOutput::debug_dest_t::ACC);
    
    adc_init();
    messages_init();

    printf("\n\nZC95 Startup, firmware version: %s\n", kGitHash);

    mutex_init(&gI2cMutex);
    debug_counters_init();

    // I2C Initialisation
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    CHwCheck hw_check(&batteryGauge);
    hw_check.check_part1(); // If a fault is found, this never returns
    audio.set_audio_digipot_found(hw_check.audio_digipot_found());
    
    // switch off backlight until init done
    port_expander.set_lcd_backlight(false);

    // Make sure there is some semi-random-ish data available
    seed_random_from_rosc();

    // Note eeprom ic is on i2c bus
    sleep_ms(100); // wait for eeprom to be ready
    CSavedSettings settings = CSavedSettings(&eeprom);
    g_SavedSettings = &settings;

    // Configure AUX port for serial or audio use
    if (settings.get_aux_port_use() == CSavedSettings::setting_aux_port_use::AUDIO)
    {
        //sleep_ms(100); // wait for eeprom to be ready
        port_expander.audio_input_enable(true);
    }
    else
    {
        port_expander.audio_input_enable(false);
    }

    CDebugOutput::set_debug_destination_from_settings(&settings);

    if (hw_check.get_front_panel_version() == CHwCheck::front_panel_version_t::v0_2)
        _front_panel = new CFrontPanelV02(&settings);
    else
        _front_panel = new CFrontPanelV01(&settings, &port_expander);

    // Front panel LEDs - give some feedback we're powering up (display takes almost second to appear)
    CLedControl led = CLedControl(PIN_LED, &settings);
    led.init();
    led.set_all_led_colour(LedColour::Purple);
    led.loop();

    radio = new CRadio(&analogueCapture);
    CBluetooth bluetooth = CBluetooth(radio);

    // front panel push buttons on controls port expander
    gpio_init(PIN_CONTROLS_INT);
    gpio_set_dir(PIN_CONTROLS_INT, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_CONTROLS_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    port_expander.clear_input();
    port_expander.process(true);

    // Front panel
    gpio_init(PIN_FP_INT1);
    gpio_init(PIN_FP_INT2);
    gpio_set_dir(PIN_FP_INT1, GPIO_IN);
    gpio_set_dir(PIN_FP_INT2, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT1, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT2, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  
    // Configure SPI display
    CDisplay display = CDisplay(_front_panel, &bluetooth);
    display.init(); // This takes some time - not far off a second
    hw_check.set_display(&display);

    // Show the splash screen & start the update of the display via DMA. To avoid briefly
    // showing rubbish, wait for that inital update to complete, then turn on the backlight.
    display.show_splash_screen(); 
    sleep_ms(25);
    port_expander.set_lcd_backlight(true);

    // Get list of available patterns / routines
    std::vector<CRoutines::Routine> routines;
    CRoutines::get_routines(routines);
   
    hw_check.check_part2(&led, &port_expander); // If a fault is found, this never returns

    // Queue used for pulses from audio processing on core0 being sent to core1 for output
    for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        queue_init(&gPulseQueue[channel], sizeof(pulse_message_t), PULSE_QUEUE_LENGTH);

    // Queue used for routines (so far just Lua) running on Core1 to send debug messages (via print())
    // out over a websocket connection, if running via RemoteAccess
    queue_init(&gPatternTextOutputQueue, sizeof(pattern_text_output_t), PATTERN_TEXT_OUTPUT_QUEUE_LENGTH);

    // Queue used to send bluetooth HID events to Lua scripts
    queue_init(&gBtRawHidQueue , sizeof(CBluetoothConnect::bt_raw_hid_queue_entry_t), 15);

    // Load/set gain, mic preamp, etc., from eeprom
    audio.init(&settings, &display);

    analogueCapture.start();

    led.set_all_led_colour(LedColour::Black);

    sleep_ms(100);

    core1_start(routines, &settings);
    CRoutineOutput* routine_output = new CRoutineOutputCore1(&display, &led, &ext_input, &audio);

    audio.set_routine_output(routine_output);
    wifi = new CWifi(radio, &analogueCapture, routine_output, routines);
    flash_helper_init(&analogueCapture, routine_output);

    // Configure port expander used for external inputs (accessory & trigger sockets)
    ext_input = new CExtInputPortExp(EXT_INPUT_PORT_EXP_ADDR, &led, routine_output);
    gpio_init(PIN_EXT_INPUT_INT);
    gpio_set_dir(PIN_EXT_INPUT_INT, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_EXT_INPUT_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    ext_input->clear_input();
    ext_input->process(true);


    CMainMenu routine_selection = CMainMenu(&display, routines, &port_expander, &settings, routine_output, &hw_check, &audio, &analogueCapture, wifi, &bluetooth, radio);
    routine_selection.show();
    CMenu *current_menu = &routine_selection;
    display.set_current_menu(current_menu);

    uint64_t start = time_us_64();
    led.loop();
    //port_expander.set_lcd_backlight(true);
    uint64_t last_analog_check = 0;
    display.set_battery_percentage(batteryGauge.get_battery_percentage());

    while (1) 
    {

        uint64_t loop_start = time_us_64();
        radio->loop();
        wifi->loop();

        display.update();
        process_front_panel_input(&port_expander, current_menu);

        int8_t adj = _front_panel->get_adjust_control_change();
        if (adj && current_menu)
        {
            current_menu->adjust_rotary_encoder_change(adj);
        }

        if (time_us_64() > last_analog_check + 50000)
        {
            _front_panel->process(true);
            last_analog_check = time_us_64();
        }
    
        update_power_levels_from_front_panel(routine_output);
        if (time_us_64() - start > 1000000) // every second
        {
            start = time_us_64();
            port_expander.process(true);   // ~215us
            ext_input->process(true); // ~215us
            led.loop(true); // ~55u

            uint64_t timenow = time_us_64();
            uint8_t batt_percentage = batteryGauge.get_battery_percentage();
            // printf("Loop time: %" PRId64 ", batt: %d\n", timenow - loop_start, batt_percentage);
            display.set_battery_percentage(batt_percentage);
            
        }
        else
        {
            ext_input->process(false);
            port_expander.process(false);
            _front_panel->process(false);     
         //   hw_check.process();
        }

        routine_output->loop();
        led.loop();
        analogueCapture.process();
        audio.process();

        if (audio.is_audio_update_available(true))
        {
            display.set_update_required();
        }

        if (analogueCapture.new_battery_readings_available())
        {
            uint8_t readings_count = 0;
            uint8_t *readings = analogueCapture.get_battery_readings(&readings_count);
            batteryGauge.add_raw_adc_readings(readings, readings_count);
        }

        if (g_SerialConnection)
            g_SerialConnection->loop();

        if (gFatalError)
        {
            routine_output->stop_routine();
            hw_check.die(&led, gErrorString); // never returns
        }
    }

    return 0;
}
