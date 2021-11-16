#ifndef _CZCOUTPUT_H
#define _CZCOUTPUT_H

#include "hardware/spi.h"
#include "CZC1Comms.h"
#include "../CSimpleOutputChannel.h"
#include "../CInternalOutputChannel.h"
#include "../../../CSavedSettings.h"


class CZC1Channel : public CSimpleOutputChannel
{


    public:
        CZC1Channel(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id);
        ~CZC1Channel();
        
        bool is_internal();
        void send_message_test();


        void on();
        void off();
        void set_power(uint16_t power);
        void pulse(uint16_t minimum_duration_ms);
        void loop(uint64_t time_us);

    private:
        void send_pulse_message(uint8_t pulse_width);

        CZC1Comms *_comms;
        uint8_t _channel_id;
        uint16_t _interval_ms;
        uint64_t _next_pulse_time;
        uint64_t _off_time;
        bool _on;
        
};

#endif
