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
        
        // Two methods on control. Either:
        // 1. Send individual pulses:         
        virtual void channel_pulse(uint8_t pos_us, uint8_t neg_us) = 0;

        // 2. Set frequency & pulse width, then turn on/off
        virtual void set_freq(uint16_t freq_hz) = 0;
        virtual void set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us) = 0;
        virtual void on() = 0;
        virtual void off() = 0;


        virtual void loop(uint64_t time_us) {};
        virtual void init() {};
        virtual void deinit() {};
        virtual bool set_channel_isolation(bool on) = 0;

        COutputChannel::channel_type get_channel_type()
        {
            return COutputChannel::channel_type::FULL;
        }

        virtual ~CFullOutputChannel() {};

    protected:
        friend class CFullChannelAsSimpleChannel;
        virtual void set_absolute_power(uint16_t power) = 0;

};

#endif
