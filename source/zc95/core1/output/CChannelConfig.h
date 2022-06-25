#ifndef _CCHANNELCONFIG_H
#define _CCHANNELCONFIG_H

#include "../display/CDisplay.h"
#include "../config.h"
#include "../CSavedSettings.h"
#include "../CLedControl.h"

#include "COutputChannel.h"
#include "../CPowerLevelControl.h"

#include "mk312/COutput312b.h"

#include "collar/CCollarComms.h"
#include "collar/CCollarChannel.h"

#include "zc1Output/CZC1ChannelFull.h"
#include "zc1Output/CZC1Comms.h"

class CChannelConfig
{

    public:
        CChannelConfig(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control);
        ~CChannelConfig();

        void configure_channels_from_saved_config(COutputChannel** active_channels);
        void loop();

        CCollarComms *get_collar_comms();

    private:
        CSavedSettings *_saved_settings;
        CPowerLevelControl *_power_level_control;
        CCollarComms _collar_comms = CCollarComms(PIN_433TX); // 433MHz transmitter for collars
    
        CZC1Comms _zc1_comms = CZC1Comms(spi1, I2C_PORT);    
};



#endif  

