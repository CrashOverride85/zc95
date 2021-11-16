#ifndef _CFULLOUTPUTCHANNEL_H
#define _CFULLOUTPUTCHANNEL_H

#include "COutputChannel.h"
#include "../CSavedSettings.h"
#include <inttypes.h>
#include <cmath>

class CFullOutputChannel : public COutputChannel
{
    public:
        CFullOutputChannel(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control, uint8_t channel_id) :
         COutputChannel(saved_settings, power_level_control, channel_id) {};
        virtual ~CFullOutputChannel() {};

        virtual void channel_pulse(uint8_t pos_us, uint8_t neg_us);

        virtual void loop(uint64_t time_us) {};
        virtual void init() {};
        virtual void deinit() {};

        COutputChannel::channel_type get_channel_type()
        {
            return COutputChannel::channel_type::FULL;
        }

    protected:
        friend class CFullChannelAsSimpleChannel;
        virtual void set_absolute_power(uint16_t power);

    private:
        void set_power_level(uint16_t power_level);
};

#endif
