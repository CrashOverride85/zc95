#ifndef _GDEBUGCOUNTERS_H
#define _GDEBUGCOUNTERS_H

#include <inttypes.h>
#include "pico/multicore.h"

enum dbg_counter_t
{
    DBG_COUNTER_MSG_OLD,    // pulses where the due time has passed by more than 1ms when picked up to process
    DBG_COUNTER_MSG_PAST,   // pulses received where the time has already passed
    DBG_COUNTER_MSG_FUTURE, // pulses with a timestamp too far in the future (currently > 1 second)
    DBG_COUNTER_MSG_INVALID // pulse messages that are invalid, e.g. power level > 1000
};

void debug_counters_init();
void debug_counters_reset();
void debug_counters_increment(dbg_counter_t counter);
uint32_t debug_counters_get(dbg_counter_t counter);


#endif

