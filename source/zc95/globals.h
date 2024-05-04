
#ifndef _CGLOBALS_H
#define _CGLOBALS_H
#include "pico/util/queue.h"
#include "pico/mutex.h"
#include "config.h"
#include "CSavedSettings.h"
#include <string>

extern bool gInteruptable;
extern queue_t gPulseQueue[MAX_CHANNELS];

extern queue_t gPatternTextOutputQueue;
extern queue_t gBtRawHidQueue;

extern bool gFatalError;
extern std::string gErrorString;

extern mutex_t gI2cMutex;

extern CSavedSettings *g_SavedSettings;

#endif
