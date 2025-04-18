#include "PCF8574.h"
#include <stdio.h>
#include "pico/stdlib.h"

/* This is used for the two port expanders on the main board of a MK1, 
 * and the port expander on v0.1 of the front panel.
 */

PCF8574::PCF8574(uint8_t i2c_address)
{
    _address = i2c_address;
}

bool PCF8574::read_port_expander(uint8_t *value)
{
    uint8_t buffer[1];
    int retval = i2c_read(__func__, _address, buffer, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
        printf("PCF8574::read_port_expander i2c read error!\n");
        return false;
    }

    if (value)
        *value = buffer[0];
    _input_state = buffer[0];
    return true;
}

bool PCF8574::write_port_expander(uint8_t value)
{
    int retval = i2c_write(__func__, _address, &value, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
        printf("PCF8574::write_port_expander i2c write error! (%d)\n", retval);
        return false;
    }
    return true;
}

bool PCF8574::set_pin_as_output(uint8_t pin)
{
    // A PCF8574 has no real concept of this (no direction register). You just call set_pin_state.
    return true;
}

bool PCF8574::set_pin_state(uint8_t pin, bool high)
{
    uint8_t new_val = _output_state;

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
        _output_state = new_val;
        return true;
    }

    return false;
}

bool PCF8574::is_output_pin_set(uint8_t pin)
{
    return _output_state & (1 << pin);
}

bool PCF8574::get_pin_state(uint8_t pin)
{
    return _input_state  & (1 << pin);
}
