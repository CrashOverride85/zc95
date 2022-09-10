
#ifndef _CGLOBALS_H
#define _CGLOBALS_H
#include "pico/util/queue.h"
#include "config.h"

extern bool gInteruptable;
extern queue_t gPulseQueue[MAX_CHANNELS];

#endif
