#ifndef _CCHANNELCONFIG_H
#define _CCHANNELCONFIG_H

#include "../display/CDisplay.h"
#include "../../common/zc95_config.h"
#include "../CSavedSettings.h"
#include "../CLedControl.h"

#include "COutputChannel.h"
#include "../CPowerLevelControl.h"

#include "collar/CCollarComms.h"
#include "collar/CCollarChannel.h"
#include "ZC624Output/CZC624Channel.h"
#include "ZC624Output/CZC624Comms.h"
#include "dummy_output/CDummyOutput.h"
#include <core1/routines/CRoutine.h>

class CChannelConfig
{

    public:
        CChannelConfig(CSavedSettings *saved_settings);
        ~CChannelConfig();

        void configure_channels(std::vector<COutputChannel*>* active_channels, std::vector<channel_config_t>& chanel_conf);
        void configure_channels_from_saved_config(std::vector<COutputChannel*>* active_channels);
        void clear_chanel_config(std::vector<COutputChannel*>* active_channels);
        void loop();
        CPowerLevelControl* PowerLevelControl();

        CCollarComms *get_collar_comms();
        void shutdown_zc624();

    private:
        COutputChannel* get_ouput_chanel(CChannel_types::channel_type channel_type, uint8_t channel_index, uint8_t channel_id);
        CSavedSettings *_saved_settings;
        CPowerLevelControl *_power_level_control;
        CCollarComms _collar_comms = CCollarComms(PIN_433TX); // 433MHz transmitter for collars
    
        CZC624Comms _zc624_comms = CZC624Comms(ZC624_SPI_PORT, I2C_PORT);
};

#endif  
