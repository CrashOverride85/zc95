#ifndef _CAUDIO3PROCESS_H
#define _CAUDIO3PROCESS_H

#include <inttypes.h>
#include <stdio.h>
#include "../CAnalogueCapture.h"
#include "../core1/CRoutineOutput.h"


class CAudio3Process
{
    public:
        CAudio3Process(uint8_t output_channel, CAnalogueCapture *analogue_capture);
        void process_samples(uint16_t sample_count, uint8_t *buffer, uint8_t *out_intensity);

    private:
        void process_sample(uint64_t time_us, int16_t value);
        uint64_t get_zero_cross_time(uint64_t time0, int16_t value0, uint64_t time1, int16_t value1);

        uint8_t _output_channel;
        CAnalogueCapture *_analogue_capture;
        int16_t _last_sample_value = 0;
        uint64_t _last_sample_time_us = 0;

        uint64_t _last_pulse_us = 0;
};

#endif
