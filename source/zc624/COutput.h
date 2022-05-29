#ifndef _COUTPUT_H
#define _COUTPUT_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "config.h"
#include "pulse_gen.pio.h"
#include "COutputChannel.h"
#include "CMsgDac.h"


class COutput
{
    public:
        COutput(PIO pio);
        ~COutput();
        
        void pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us);
        void set_power(uint8_t channel, uint16_t power);
        void set_freq(uint8_t channel, uint16_t freq);
        void set_pulse_width(uint8_t channel, uint8_t pos, uint8_t neg);
        void on(uint8_t channel);
        void off(uint8_t channel);

    private:
        bool is_channel_valid(uint8_t channel);
        void setup_gpio(uint8_t pin);
        COutputChannel *_channel[4];
        PIO _pio;
        uint _pio_program_offset;
        CMsgDac _dac = CMsgDac();
};

#endif
