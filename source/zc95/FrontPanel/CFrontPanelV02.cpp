#include "CFrontPanelV02.h"
#include <string.h>
#include "../globals.h"
#include "../CUtil.h"

/*
 * v0.2 Front panel which has:
   -  ADS1115 ADC connected to 4x potentiometers
   -  TCA9534 port expander, connected to:
      - 4 buttons
      - rotary encoder (with push button)
   -  TLC59108 LED driver for the 4 illuminated buttons (unrelated to the 6x WS2812D LEDs)
 */

CFrontPanelV02::CFrontPanelV02(CSavedSettings *saved_settings)
{
    memset(_power_level, 0, sizeof(_power_level));
    _last_port_exp_read = 0xFF; // Assume buttons aren't pressed to start with
    _adjust_value = 0;
    _button_states_at_last_check = 0;
    _saved_settings = saved_settings;
    memset(_last_state_change, 0, sizeof(_last_state_change));

    init_adc();
    init_port_exp();

    // Enable LED outputs 0-3 (only ones connected), and allow dimming via PWM registers
    write_led_register(led_reg_t::LEDOUT0, 0xFF); 

    write_led_register(led_reg_t::PWM0, 10);
    write_led_register(led_reg_t::PWM1, 10);
    write_led_register(led_reg_t::PWM2, 10);
    write_led_register(led_reg_t::PWM3, 10);

}

void CFrontPanelV02::init_adc()
{
    // Enable continuous-conversion
    _adc_config_reg &= ~((uint16_t)1 << 8); // Clear bit 8, "MODE" 

    // Set PGA (Programmable gain amplifier) configuration to b001 (+/-4.096V). Default is b010  (+/-2.048V)
    _adc_config_reg &= ~((uint16_t)7 << 9);    // clear bits 11:9 
    _adc_config_reg |= ((uint16_t)1 << 9);     // set bit 9 to 1

    _adc_config_reg = update_adc_input(adc_input_t::AIN0);
    _adc_selected_input = adc_input_t::AIN0;
    write_adc_register(adc_reg_t::CONFIG, _adc_config_reg);
}

void CFrontPanelV02::write_adc_register(adc_reg_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = (uint8_t)reg;
    buf[1] = value >> 8;
    buf[2] = value & 0xFF;
    i2c_write(__func__, FP_0_2_ADC_ADDR, buf, sizeof(buf), false);
}

bool CFrontPanelV02::read_adc_register(adc_reg_t reg, uint16_t *value)
{
    uint8_t command = (uint8_t)reg;
    int bytes_written = i2c_write(__func__, FP_0_2_ADC_ADDR, &command, 1, false);
    if (bytes_written != 1)
    {
        printf("CFrontPanelV02::read_adc_register() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    uint8_t buf[2] = {0};
    int retval = i2c_read(__func__, FP_0_2_ADC_ADDR, buf, sizeof(buf), false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFrontPanelV02::read_adc_register() i2c read error!\n");
      return false;
    }

    *value = (buf[0] << 8) + buf[1];
    return true; 
}

uint16_t CFrontPanelV02::update_adc_input(adc_input_t input)
{
    uint16_t new_reg_val = _adc_config_reg;
    new_reg_val &= ~((uint16_t)7 << 12);    // clear bits 14:12
    new_reg_val |= ((uint16_t)input << 12); // set bits 14:12 based on input to read
    return new_reg_val;
}

void CFrontPanelV02::adc_select_next_input()
{
    if (_adc_selected_input == adc_input_t::AIN0)
        _adc_selected_input = adc_input_t::AIN1;
    else if (_adc_selected_input == adc_input_t::AIN1)
        _adc_selected_input = adc_input_t::AIN2;
    else if (_adc_selected_input == adc_input_t::AIN2)
        _adc_selected_input = adc_input_t::AIN3;
    else if (_adc_selected_input == adc_input_t::AIN3)
        _adc_selected_input = adc_input_t::AIN0;

    _adc_config_reg = update_adc_input(_adc_selected_input);
    write_adc_register(adc_reg_t::CONFIG, _adc_config_reg);
}

void CFrontPanelV02::set_button_in_use(enum Button button, bool in_use) 
{
    _buttons_in_use &= ~(1 << (uint8_t)button); // clear bit
    
    if (in_use)
        _buttons_in_use |= (1 << (uint8_t)button); // set
}

void CFrontPanelV02::write_led_register(led_reg_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)reg;
    buf[1] = value;
    i2c_write(__func__, FP_0_2_BUTTON_LED_DRV_ADDR, buf, sizeof(buf), false);
}

void CFrontPanelV02::update_button_led_states()
{
    static uint8_t button_states = 0;
    static uint8_t brightness = 0;

    bool brightness_changed = brightness != _saved_settings->get_button_brightness();

    if (button_states != _buttons_in_use || brightness_changed)
    {
        update_button_led_state(Button::A, button_states, _buttons_in_use, led_reg_t::PWM0, brightness_changed);
        update_button_led_state(Button::B, button_states, _buttons_in_use, led_reg_t::PWM1, brightness_changed);
        update_button_led_state(Button::C, button_states, _buttons_in_use, led_reg_t::PWM2, brightness_changed);
        update_button_led_state(Button::D, button_states, _buttons_in_use, led_reg_t::PWM3, brightness_changed);

        button_states = _buttons_in_use;
        brightness = _saved_settings->get_button_brightness();
    }
}

void CFrontPanelV02::update_button_led_state(enum Button button, uint8_t old_state, uint8_t new_state, led_reg_t reg, bool always_update)
{
    bool button_active_old = old_state & (1 << (uint8_t)button);
    bool button_active_new = new_state & (1 << (uint8_t)button);

    if (button_active_old != button_active_new || always_update)
    {
        if (button_active_new)
        {
            write_led_register(reg, _saved_settings->get_button_brightness());
        }
        else
        {
            write_led_register(reg, 0);
        }
    }
}

void CFrontPanelV02::interrupt(interrupt_t i)
{
    // The INT1 pin on front panel v0.2 isn't connected to anything, so we 
    // shouldn't be getting any INT1 interrupts, but if we do, ignore them
    if (i == interrupt_t::INT2)
    {
        _interrupt = true;
        _interrupt_time = time_us_32();

        if (gInteruptable)
            process(false);
    }
}

void CFrontPanelV02::process(bool always_update)
{
    if (_interrupt || always_update)
    {
        if (_interrupt)
        {
            //printf("delay=%lu us\n", time_us_32() - _interrupt_time);
            _interrupt = false;
        }
        
        if (always_update)
        {
            read_adc();
        }
       
        uint8_t buffer;
        bool read_success = read_port_expander(&buffer);

        if (read_success && _last_port_exp_read != buffer)
        {
            rot_encoder_process(buffer, port_exp_pin_t::ROT_A, port_exp_pin_t::ROT_B);

            if (abs(_adjust_value < 100))
                _adjust_value += _rot_encoder.get_rotary_encoder_change();

            _last_port_exp_read = buffer;
        }
    }

    update_button_led_states();
}

uint16_t CFrontPanelV02::get_channel_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _power_level[channel];
}

int8_t CFrontPanelV02::get_adjust_control_change()
{
    int8_t retval = _adjust_value;
    _adjust_value = 0;
    return retval;
}

void CFrontPanelV02::read_adc()
{
    adc_input_t adc_chanel_read = _adc_selected_input;
    uint16_t val;
    if (!read_adc_register(adc_reg_t::CONVERSION, &val))
    {
        // Failing to read from the ADC (having successfully read it during the hw check at power on) might
        // suggest a loose connection. It's probably best to stop because a poor connection could lead 
        // unexpected power levels being set. 

        printf("Error reading front panel ADC\n");
        gErrorString = "Front panel fault, \nerror reading ADC.";
        gFatalError = true;

        return;
    }

    int16_t adc_value = (int16_t)val;
    if (adc_value < 0)
        adc_value = 0;
    adc_select_next_input(); // so the next call to read_adc() will read the next channel
   
    // With the current PGA configuration (set in init_adc), a full scale reading of 4.096v would be 32768
    // 32768 / 4.096 = 8000      => 8000 per volt
    // Supply voltage is supposed to be 3.3, but assume 3.25 here. So max reading expected is 8000 * 3.25 = 26000
    // Need to convert 0..26000 to 0..1000, so 1000/26000 = 0.038461538
    const double multiplier = 0.038461538;

    int16_t adj_adc_read = (double)adc_value * multiplier;
    if (adj_adc_read < 0)
        adj_adc_read = 0;
    if (adj_adc_read > 1000)
        adj_adc_read = 1000;

    // With a dial turned fully clockwise, adj_adc_read will be 0, fully counter-clockwise it will be 1000 (or thereabouts) 
    uint16_t power_level = 1000 - adj_adc_read;

    switch (adc_chanel_read)
    {
        case adc_input_t::AIN0:
            _power_level[3] = power_level;
            break;

        case adc_input_t::AIN1:
            _power_level[2] = power_level;
            break;

        case adc_input_t::AIN2:
            _power_level[1] = power_level;
            break;

        case adc_input_t::AIN3:
            _power_level[0] = power_level;
            break;
    }
}

////////////////////////

void CFrontPanelV02::init_port_exp()
{
    uint8_t txbuf[2] = {0};

    txbuf[0] = port_exp_reg_t::CONFIGURATION;
    txbuf[1] = ~(1 << port_exp_pin_t::LED_RESET); // Set p4 (only) to output
    i2c_write(__func__, FP_0_2_PORT_EXP_ADDR, txbuf, sizeof(txbuf), false);
}

bool CFrontPanelV02::read_port_expander(uint8_t *value)
{
    uint8_t buffer[1] = {0};
    
    buffer[0] = port_exp_reg_t::INPUT_PORT;
    int bytes_written = i2c_write(__func__, FP_0_2_PORT_EXP_ADDR, buffer, 1, false);
    if (bytes_written != 1)
    {
        printf("CFrontPanelV02::read_port_expander() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    int retval = i2c_read(__func__, FP_0_2_PORT_EXP_ADDR, buffer, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFrontPanelV02::read_port_expander() i2c read error!\n");
      return false;
    }

    *value = buffer[0];
    return true;
}

void CFrontPanelV02::rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos)
{
    uint8_t a = (port_exp_read & (1 << a_pos) ? 1 : 0);
    uint8_t b = (port_exp_read & (1 << b_pos ) ? 1 : 0);

    _rot_encoder.process(a, b);
}

bool CFrontPanelV02::has_button_state_changed(enum Button button, bool *new_state)
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

bool CFrontPanelV02::button_state(enum Button button)
{
    return !(_last_port_exp_read & (1 << (uint8_t)button));
}
