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
        void pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us, uint64_t delay_until_us = 0);
        void set_power(uint8_t channel, uint16_t power);
        void set_freq(uint8_t channel, uint16_t freq);
        void set_pulse_width(uint8_t channel, uint8_t pos, uint8_t neg);
        void on(uint8_t channel);
        void off(uint8_t channel);
        void loop();
        void power_down();
        uint8_t get_channel_led_state();
        void sync_chanel(uint8_t channel_lead, uint8_t channel_linked, uint8_t offset_percent);

    private:
        struct triphase_conf_t
        {
            uint8_t linked_channel;
            uint8_t offset_percent;
        };

        void reset_chanel_triphase(uint8_t chan);
        bool is_linked_channel(uint8_t channel);
        void unlink_if_linked(uint8_t channel);
        bool is_channel_valid(uint8_t channel);
        void setup_gpio(uint8_t pin);
        COutputChannel *_channel[4];
        triphase_conf_t _channel_triphase[4];
        PIO _pio;
        uint _pio_program_offset;
        CMsgDac _dac = CMsgDac();
        CI2cSlave *_i2c_slave;
        CPulseQueue *_pulse_queue = NULL;
        bool _channel_isolation_last_value;
};

#endif
