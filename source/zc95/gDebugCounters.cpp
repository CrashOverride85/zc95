#include "gDebugCounters.h"

static mutex_t  _dbg_counter_mutex;
static uint32_t _dbg_discard_old_counter = 0;
static uint32_t _dbg_discard_past_counter = 0;
static uint32_t _dbg_discard_future_counter = 0;
static uint32_t _dbg_discard_invalid_counter = 0;

void debug_counters_init()
{
    mutex_init(&_dbg_counter_mutex);
    debug_counters_reset();
}

void debug_counters_reset()
{
    mutex_enter_blocking(&_dbg_counter_mutex);
    _dbg_discard_old_counter     = 0;
    _dbg_discard_past_counter    = 0;
    _dbg_discard_future_counter  = 0;
    _dbg_discard_invalid_counter = 0;
    mutex_exit(&_dbg_counter_mutex);
}

void debug_counters_increment(dbg_counter_t counter)
{
    mutex_enter_blocking(&_dbg_counter_mutex);

    switch(counter)
    {
        case dbg_counter_t::DBG_COUNTER_MSG_FUTURE:
            _dbg_discard_future_counter++;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_OLD:
            _dbg_discard_old_counter++;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_PAST:
            _dbg_discard_past_counter++;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_INVALID:
            _dbg_discard_invalid_counter++;
            break;
    }

    mutex_exit(&_dbg_counter_mutex);
}

uint32_t debug_counters_get(dbg_counter_t counter)
{
    uint32_t value = 0;
    switch(counter)
    {
        case dbg_counter_t::DBG_COUNTER_MSG_FUTURE:
            value = _dbg_discard_future_counter;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_OLD:
            value = _dbg_discard_old_counter;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_PAST:
            value = _dbg_discard_past_counter;
            break;

        case dbg_counter_t::DBG_COUNTER_MSG_INVALID:
            value = _dbg_discard_invalid_counter;
            break;
    }

    return value;
}
