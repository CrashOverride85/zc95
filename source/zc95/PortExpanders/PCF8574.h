#ifndef _PCF8574_H
#define _PCF8574_H

#include <inttypes.h>
#include <map>
#include "IPortExpander.h"
#include "../ZcTypes.h"
#include "../CUtil.h"

class PCF8574 : public IPortExpander
{
    public:
        typedef void(* port_expander_callback_t) (void* user, uint8_t gpio, bool pin_state);

        PCF8574(uint8_t i2c_address);
        bool read_port_expander(uint8_t *value);
        bool write_port_expander(uint8_t value);
        bool set_pin_as_output(uint8_t pin);
        bool set_pin_state(uint8_t pin, bool high);
        bool is_output_pin_set(uint8_t pin);
        bool get_pin_state(uint8_t pin);

    private:
        uint8_t _address;
        uint8_t _output_state = 0xFF;
        uint8_t _input_state = 0;
        volatile bool _interrupt = false;

};

#endif
