#ifndef _CZCOUTPUT_H
#define _CZCOUTPUT_H

#include "hardware/spi.h"
#include "CZC1Comms.h"
#include "../CSimpleOutputChannel.h"
#include "../CInternalOutputChannel.h"
#include "../../../CSavedSettings.h"


class CZC1ChannelSimple : public CSimpleOutputChannel
{


    public:
        CZC1ChannelSimple(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id);
        ~CZC1ChannelSimple();
        
        bool is_internal();
        void send_message_test();

        void on();
        void off();
        void set_power(uint16_t power);
        void pulse(uint16_t minimum_duration_ms);
        void loop(uint64_t time_us);

    private:
        CZC1Comms *_comms;
        uint8_t _channel_id;
        uint64_t _off_time;
        bool _on; 
};

#endif
