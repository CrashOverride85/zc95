#include "CPulseQueue.h"

/*
 * Queue up pulses to be generated. This is to ensure that only one chanel is ever 
 * generating a pulse at the exact same moment.
 * This for (largely theoretical) safety reasons, rather than power use, etc.
 * For the time being, this rules out any kind of tri-phase effects.
 */

CPulseQueue::CPulseQueue(CI2cSlave *i2c_slave)
{
    _i2c_slave = _i2c_slave;
    queue_init(&_pulse_queue, sizeof(element_t), 10);
    _next_pulse = 0;
    _channel_isolation_last_value = _i2c_slave->get_value(CI2cSlave::reg::ChannelIsolation);
    printf("CPulseQueue::CPulseQueue(): ChannelIsolation: %d\n", _channel_isolation_last_value);
}

CPulseQueue::~CPulseQueue()
{
    queue_free(&_pulse_queue);
}

void CPulseQueue::queue_pulse(uint sm, uint8_t pos, uint8_t neg)
{
    element_t element = 
    {
            .sm = sm,
            .pos_us = pos,
            .neg_us = neg
    };    
    
    if (!queue_try_add(&_pulse_queue, &element))
    {
        printf("CPulseQueue: queue full\n");
    }
}

bool CPulseQueue::get_queued_pulse(uint *sm, uint8_t *pos, uint8_t *neg)
{
    bool channel_isolation = _i2c_slave->get_value(CI2cSlave::reg::ChannelIsolation);
    if (channel_isolation != _channel_isolation_last_value)
    {
        printf("CPulseQueue - ChannelIsolation: %s\n", channel_isolation ? "ENABLED" : "DISABLED");
        _channel_isolation_last_value = channel_isolation;
    }

    if (queue_is_empty(&_pulse_queue))
        return false;  
   
    if (channel_isolation)
    {
        // If channel isolation is on, we need to seperate pulses time-wise (the whole point of this class)
        if (time_us_64() < _next_pulse)
            return false;
    }

    element_t element;
    queue_remove_blocking(&_pulse_queue, &element);

    _next_pulse = time_us_64() + element.pos_us + element.neg_us + 150;
    *sm =  element.sm;
    *pos = element.pos_us;
    *neg = element.neg_us;
    return true;
}
