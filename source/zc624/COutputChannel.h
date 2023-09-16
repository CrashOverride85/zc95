#ifndef _COUTPUTCHANNEL_H
#define _COUTPUTCHANNEL_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "CDac.h"
#include "CPulseQueue.h"

class COutputChannel
{
    public:
        enum class status
        {
            INITIAL,
            READY,
            FAULT
        };

        COutputChannel(uint8_t pin_gate_a, PIO pio, uint sm, uint pio_program_offset, uint8_t adc, CDac *dac, CDac::dac_channel dac_channel, CPulseQueue *pulse_queue);
        ~COutputChannel();
        bool calibrate();
        void set_power(uint16_t power);
        void set_freq(uint16_t freq);
        void set_pulse_width(uint8_t pos, uint8_t neg);
        void on();
        void off();
        void queue_pulse(uint8_t pos_us, uint8_t neg_us);
        void do_pulse(uint8_t pos_us, uint8_t neg_us);
        status get_status();
        void diag_run_dac_sweep();
        bool get_channel_led();

    private:
        static int cmpfunc (const void * a, const void * b);
        float get_adc_voltage();
        static bool s_timer_callback(repeating_timer_t *rt);
        bool timer_callback(repeating_timer_t *rt);
        void channel_led_on();
        
        status _status;
        uint8_t _pin_gate_a;
        PIO _pio;
        uint _sm;
        uint8_t _adc;
        CDac *_dac;
        CDac::dac_channel _dac_channel;
        uint16_t _cal_value;
        uint _pio_program_offset;

        repeating_timer_t _timer;
        uint16_t _freq;
        bool _freq_changed;
        uint8_t _pulse_width_neg_us;
        uint8_t _pulse_width_pos_us;
        bool _on;
        CPulseQueue *_pulse_queue;
        volatile uint64_t _channel_led_off_time_us = 0;
};

#endif
