#ifndef _CSIMPLEOUTPUTCHANNEL_H
#define _CSIMPLEOUTPUTCHANNEL_H

#include "COutputChannel.h"
#include "../CSavedSettings.h"
#include <inttypes.h>
#include <cmath>

class CSimpleOutputChannel : public COutputChannel
{
    public:
        CSimpleOutputChannel(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control, uint8_t channel_id) :
         COutputChannel(saved_settings, power_level_control, channel_id) {};
        virtual ~CSimpleOutputChannel() {};
        
        void channel_pulse(uint16_t minimum_duration_ms)
        {
            pulse(minimum_duration_ms);
        }

        void channel_off()
        {
            off();
        }

        void channel_on()
        {
            on();
        }


        virtual void loop(uint64_t time_us) {};
        virtual void init() {};
        virtual void deinit() {};

        COutputChannel::channel_type get_channel_type()
        {
            return COutputChannel::channel_type::SIMPLE;
        }

    protected:
        virtual void on() = 0;
        virtual void off() = 0;
        virtual void pulse(uint16_t minimum_duration_ms) = 0;
        virtual void set_absolute_power(uint16_t power) = 0;

    private:
        void set_power_level(uint16_t power_level);
};

#endif
