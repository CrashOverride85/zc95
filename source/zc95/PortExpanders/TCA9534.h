#ifndef _TCA9534_H
#define _TCA9534_H

#include <inttypes.h>
#include <map>
#include "IPortExpander.h"
#include "../ZcTypes.h"
#include "../CUtil.h"

class TCA9534 : public IPortExpander
{
    public:


        TCA9534(uint8_t i2c_address);
        bool read_port_expander(uint8_t *value);
        bool write_port_expander(uint8_t value);
        bool set_pin_as_output(uint8_t pin);
        bool set_pin_as_input(uint8_t pin);
        bool set_pin_state(uint8_t pin, bool high);
        bool is_output_pin_set(uint8_t pin);
        bool get_pin_state(uint8_t pin);

    private:
        enum port_exp_reg_t
        {
            INPUT_PORT,
            OUTPUT_PORT,
            POLARITY_INVERSION,
            CONFIGURATION
        };

        void pin_changed(uint8_t pin);
        bool is_pin_configured_as_output(uint8_t pin);

        uint8_t _i2c_address;

        // Init with defaults from datasheet
        uint8_t _config_register = 0xFF; 
        uint8_t _output_register = 0xFF;
        uint8_t _input_register  = 0x00;
};

#endif
