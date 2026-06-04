#include "TCA9534.h"
#include <stdio.h>
#include "pico/stdlib.h"

TCA9534::TCA9534(uint8_t i2c_address)
{
    _i2c_address = i2c_address;

    uint8_t txbuf[2] = {0};

    txbuf[0] = port_exp_reg_t::CONFIGURATION;
    txbuf[1] = _config_register;
    i2c_write(__func__, i2c_address, txbuf, sizeof(txbuf), false);
}

void TCA9534::pin_changed(uint8_t pin)
{
    bool debug = true;
    bool pin_state = _input_register & (1 << (uint8_t)pin);
    if (debug) printf("TCA9534 pin change: %d - %d\n", pin, pin_state);
}

bool TCA9534::set_pin_as_output(uint8_t pin)
{
    uint8_t txbuf[2] = {0};

    txbuf[0] = port_exp_reg_t::CONFIGURATION;
    txbuf[1] = _config_register;
    txbuf[1] &= ~(1 << pin);
    int bytes_written = i2c_write(__func__, _i2c_address, txbuf, sizeof(txbuf), false);
    if (bytes_written != 2)
    {
        printf("TCA9534::set_pin_as_output() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    _config_register = txbuf[1];
    return true;
}

bool TCA9534::set_pin_as_input(uint8_t pin)
{
    uint8_t txbuf[2] = {0};

    txbuf[0] = port_exp_reg_t::CONFIGURATION;
    txbuf[1] = _config_register;
    txbuf[1] |= (1 << pin);
    int bytes_written = i2c_write(__func__, _i2c_address, txbuf, sizeof(txbuf), false);
    if (bytes_written != 2)
    {
        printf("TCA9534::set_pin_as_input() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    _config_register = txbuf[1];
    return true;
}

bool TCA9534::is_pin_configured_as_output(uint8_t pin)
{
    return !(_config_register & (1 << pin));
}

bool TCA9534::read_port_expander(uint8_t *value)
{
    uint8_t buffer[1] = {0};
    
    buffer[0] = port_exp_reg_t::INPUT_PORT;
    int bytes_written = i2c_write(__func__, _i2c_address, buffer, 1, false);
    if (bytes_written != 1)
    {
        printf("TCA9534::read_port_expander() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    int retval = i2c_read(__func__, _i2c_address, buffer, 1, false);
    if (retval == -1)
    {
      printf("TCA9534::read_port_expander() i2c read error!\n");
      return false;
    }

    if (value)
        *value = buffer[0];
    _input_register = buffer[0];
    return true;
}

bool TCA9534::write_port_expander(uint8_t value)
{
    uint8_t buffer[2] = {0};

    buffer[0] = port_exp_reg_t::OUTPUT_PORT;
    buffer[1] = value;

    int bytes_written = i2c_write(__func__, _i2c_address, buffer, 2, false);
    if (bytes_written != 2)
    {
        printf("TCA9534::write_port_expander() write failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    _output_register = value;

    return true;
}

bool TCA9534::set_pin_state(uint8_t pin, bool high)
{
    uint8_t new_val = _output_register;

    if (high)
    {
        new_val |= (1 << pin);
    }
    else
    {
        new_val &= ~(1 << pin);
    }

    if (write_port_expander(new_val))
    {
        _output_register = new_val;
        return true;
    }

    return false;
}

bool TCA9534::is_output_pin_set(uint8_t pin)
{
    return _output_register & (1 << pin);
}

bool TCA9534::get_pin_state(uint8_t pin)
{
    return _input_register  & (1 << pin);
}
