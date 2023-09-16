#ifndef _COUTPUT_H
#define _COUTPUT_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "config.h"
#include "pulse_gen.pio.h"
#include "COutputChannel.h"
#include "CMsgDac.h"
#include "CI2cSlave.h"


class COutput
{
    public:
        COutput(PIO pio, CI2cSlave *i2c_slave);
        ~COutput();
        void pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us);
        void set_power(uint8_t channel, uint16_t power);
        void set_freq(uint8_t channel, uint16_t freq);
        void set_pulse_width(uint8_t channel, uint8_t pos, uint8_t neg);
        void on(uint8_t channel);
        void off(uint8_t channel);
        void loop();
        void power_down();
        uint8_t get_channel_led_state();

    private:
        bool is_channel_valid(uint8_t channel);
        void setup_gpio(uint8_t pin);
        COutputChannel *_channel[4];
        PIO _pio;
        uint _pio_program_offset;
        CMsgDac _dac = CMsgDac();
        CI2cSlave *_i2c_slave;
        CPulseQueue *_pulse_queue = NULL;
};

#endif
