#ifndef _CQUEUEPULSE_H
#define _CQUEUEPULSE_H

#include <stdio.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "CI2cSlave.h"

class CPulseQueue
{
    public:

        typedef struct element 
        {
            uint sm; // aka channel
            uint8_t pos_us;
            uint8_t neg_us;
        } element_t;

        CPulseQueue(CI2cSlave *i2c_slave);
        ~CPulseQueue();
        void queue_pulse(uint sm, uint8_t pos, uint8_t neg);
        bool get_queued_pulse(uint *sm, uint8_t *pos, uint8_t *neg);

    private:
        queue_t _pulse_queue;
        uint64_t _next_pulse;
        CI2cSlave *_i2c_slave;
        bool _channel_isolation_last_value;
};

#endif
