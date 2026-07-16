#ifndef _CZCOUTPUTCHANFULL_H
#define _CZCOUTPUTCHANFULL_H

#include "hardware/spi.h"
#include "CZC624Comms.h"
#include "../COutputChannel.h"
#include "../../../CSavedSettings.h"


class CZC624Channel : public COutputChannel
{
    public:
        CZC624Channel(CSavedSettings *saved_settings, CZC624Comms *comms, CPowerLevelControl *power_level_control, uint8_t zc624_chan_index, uint8_t channel_id);
        void loop(uint64_t time_us);
        ~CZC624Channel();
        
        void set_absolute_power(uint16_t power);
        void channel_single_pulse(uint8_t pos_us, uint8_t neg_us);
        void channel_pulse(uint16_t min_pulse_us);

        void set_freq(uint16_t freq_hz);
        void set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us);
        void on();
        void off();
        void link_channel(uint8_t channel, uint8_t offset_percentage);
        bool set_channel_isolation(bool on);

        CChannel_types::channel_type get_channel_type();

    private:
        CZC624Comms *_comms;
        uint8_t _zc624_chan_index;
        uint8_t _channel_id;
        bool _last_led_state = false;
        uint64_t _last_led_update_us = 0;
};

#endif
