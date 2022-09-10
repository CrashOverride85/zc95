#include <stdbool.h>
#include "pico/util/queue.h"
#include "config.h"

bool gInteruptable;
queue_t gPulseQueue[MAX_CHANNELS];
