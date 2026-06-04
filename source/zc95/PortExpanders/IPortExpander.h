#ifndef _IPORTEXPANDER_H
#define _IPORTEXPANDER_H

#include <inttypes.h>

typedef uint64_t time_us_t;

class IPortExpander
{
    public:
        virtual bool read_port_expander(uint8_t *value) = 0;
        virtual bool write_port_expander(uint8_t value) = 0;
        virtual bool set_pin_as_output(uint8_t pin) = 0;
        virtual bool set_pin_as_input(uint8_t pin) = 0;
        virtual bool set_pin_state(uint8_t pin, bool high) = 0;
        virtual bool is_output_pin_set(uint8_t pin) = 0;
        virtual bool get_pin_state(uint8_t pin) = 0;
        virtual ~IPortExpander() {}
};

#endif
