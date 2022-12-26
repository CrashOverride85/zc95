#include <stdbool.h>
#include <string>
#include "pico/util/queue.h"
#include "pico/mutex.h"
#include "CSavedSettings.h"
#include "config.h"


bool gInteruptable;
queue_t gPulseQueue[MAX_CHANNELS];

bool gFatalError;
std::string gErrorString;
mutex_t gI2cMutex;

CSavedSettings *g_SavedSettings = NULL;
