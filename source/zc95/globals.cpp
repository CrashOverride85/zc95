#include <stdbool.h>
#include <string>
#include "pico/util/queue.h"
#include "pico/mutex.h"
#include "CSavedSettings.h"
#include "RemoteAccess/CSerialConnection.h"
#include "config.h"


bool gInteruptable;
queue_t gPulseQueue[MAX_CHANNELS];

queue_t gPatternTextOutputQueue;

bool gFatalError;
std::string gErrorString;
mutex_t gI2cMutex;

CSavedSettings *g_SavedSettings = NULL;
CSerialConnection *g_SerialConnection = NULL;
