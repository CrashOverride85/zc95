#ifndef _CFULLCHANASSIMPLE_H
#define _CFULLCHANASSIMPLE_H

#include "hardware/spi.h"
#include "CSimpleOutputChannel.h"
#include "CFullOutputChannel.h"
#include "CInternalOutputChannel.h"
#include "../CSavedSettings.h"


class CFullChannelAsSimpleChannel : public CSimpleOutputChannel
{


    public:
        CFullChannelAsSimpleChannel(CSavedSettings *saved_settings, CFullOutputChannel *full_channel, uint8_t channel_number, CPowerLevelControl *power_level_control);
        ~CFullChannelAsSimpleChannel();
        
        bool is_internal();
        void send_message_test();

        void on();
        void off();
        void set_absolute_power(uint16_t power);
        void pulse(uint16_t minimum_duration_ms);
        void loop(uint64_t time_us);

    private:
        CFullOutputChannel *_full_channel;
        void send_pulse_message(uint8_t pulse_width);

        uint64_t _off_time;
        bool _on;        
};

#endif
