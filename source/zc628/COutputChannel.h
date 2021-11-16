#ifndef _COUTPUTCHANNEL_H
#define _COUTPUTCHANNEL_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "CDac.h"


class COutputChannel
{
    public:
        enum class status
        {
            INITIAL,
            READY,
            FAULT
        };

        COutputChannel(uint8_t pin_gate_a, PIO pio, uint sm, uint pio_program_offset, uint8_t adc, CDac *dac, CDac::dac_channel dac_channel);
        ~COutputChannel();
        bool calibrate();
        void set_power(uint16_t power);
        void pulse(uint8_t pos_us, uint8_t neg_us);
        status get_status();
        void diag_run_dac_sweep();

    private:
        static int cmpfunc (const void * a, const void * b);
        float get_adc_voltage();
        status _status;
        uint8_t _pin_gate_a;
        PIO _pio;
        uint _sm;
        uint8_t _adc;
        CDac *_dac;
        CDac::dac_channel _dac_channel;
        uint16_t _cal_value;
        uint _pio_program_offset;

};

#endif
