#include <stdbool.h>
#include <string>
#include "pico/util/queue.h"
#include "pico/mutex.h"
#include "hardware/spi.h"
#include "CSavedSettings.h"
#include "RemoteAccess/CSerialConnection.h"
#include "config.h"


bool gInteruptable;
queue_t gPulseQueue[MAX_CHANNELS];

queue_t gPatternTextOutputQueue;
queue_t gBtRawHidQueue;

bool gFatalError;
std::string gErrorString;
mutex_t gI2cMutex;

CSavedSettings *g_SavedSettings = NULL;
CSerialConnection *g_SerialConnection = NULL;

// This overrides the inbuilt weakly defined _exit that's called when things 
// go badly wrong. Instead of just stopping, try and make sure the other core 
// halts too, and the output board is sent a shutdown command.
extern "C" { 
    void _exit(int status) 
    {
    #if PICO_ENTER_USB_BOOT_ON_EXIT
        reset_usb_boot(0,0);
    #else
        gFatalError = true;

        printf("Core %d crash.\n", get_core_num());

        if (get_core_num() == 1)
        {
            // If core0 panics, the gFatalError flag being set should mean that core1 messages the zc624 
            // board to shutdown, before halting. But if this /is/ core1, we really want to have a go at 
            // doing that before halting.

            uint8_t buf[4] = {0};
            buf[0] = 3; // PowerDown command
            spi_write_blocking(ZC624_SPI_PORT, buf, sizeof(buf));
        }

        while (1) 
        {
            __breakpoint();
        }
    #endif
    }
}
