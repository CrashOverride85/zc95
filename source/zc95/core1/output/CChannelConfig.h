#ifndef _CCHANNELCONFIG_H
#define _CCHANNELCONFIG_H

#include "../display/CDisplay.h"
#include "../config.h"
#include "../CSavedSettings.h"
#include "../CLedControl.h"

#include "COutputChannel.h"
#include "../CPowerLevelControl.h"

#include "collar/CCollarComms.h"
#include "collar/CCollarChannel.h"

#include "ZC624Output/CZC624ChannelFull.h"
#include "ZC624Output/CZC624Comms.h"

class CChannelConfig
{

    public:
        CChannelConfig(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control);
        ~CChannelConfig();

        void configure_channels_from_saved_config(COutputChannel** active_channels);
        void loop();

        CCollarComms *get_collar_comms();
        void shutdown_zc624();

    private:
        CSavedSettings *_saved_settings;
        CPowerLevelControl *_power_level_control;
        CCollarComms _collar_comms = CCollarComms(PIN_433TX); // 433MHz transmitter for collars
    
        CZC624Comms _zc614_comms = CZC624Comms(ZC624_SPI_PORT, I2C_PORT);
};

#endif  
