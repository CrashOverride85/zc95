#ifndef _CZCOUTPUTCHANFULL_H
#define _CZCOUTPUTCHANFULL_H

#include "hardware/spi.h"
#include "CZC1Comms.h"
#include "../CFullOutputChannel.h"
#include "../CInternalOutputChannel.h"
#include "../../../CSavedSettings.h"


class CZC1ChannelFull : public CFullOutputChannel
{
    public:
        CZC1ChannelFull(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id);
        void loop(uint64_t time_us);
        ~CZC1ChannelFull();
        
        bool is_internal();
        void set_absolute_power(uint16_t power);
        void channel_pulse(uint8_t pos_us, uint8_t neg_us);

        void set_freq(uint16_t freq_hz);
        void set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us);
        void on();
        void off();
        bool set_channel_isolation(bool on);


    private:
        CZC1Comms *_comms;
        uint8_t _channel_id;
        uint64_t _led_off_time;
};

#endif
