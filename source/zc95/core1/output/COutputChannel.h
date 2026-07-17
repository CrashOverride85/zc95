#ifndef _COUPUTCHANNEL_H
#define _COUPUTCHANNEL_H

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "../CSavedSettings.h"
#include "../CLedControl.h"
#include "../display/CDisplay.h"
#include "../CPowerLevelControl.h"
#include "../Core1Messages.h"
#include <inttypes.h>
#include <cmath>

class COutputChannel
{
    public:
        COutputChannel(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control, uint8_t channel_id)
        {
            printf("COutputChannel(%d)\n", channel_id);
            _saved_settings = saved_settings;
            _power_level_control = power_level_control;
            _channel_id = channel_id;
        };

        virtual ~COutputChannel() 
        {
            printf("~COutputChannel(%d)\n", _channel_id);
        };

        virtual void channel_pulse(uint16_t min_pulse_ms) = 0;
        virtual void channel_single_pulse(uint8_t pos_us, uint8_t neg_us) {};
        virtual void set_freq(uint16_t freq_hz) {};
        virtual void set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us) {};
        virtual void on() {};
        virtual void off() = 0;
        virtual void link_channel(uint8_t channel, uint8_t offset_percentage) {};
        virtual bool set_channel_isolation(bool on) { return false; };
        
        // Called by routines
        void channel_set_power(uint16_t power)
        {
            _power_level_control->set_routine_requested_power_level(_channel_id, power);
            update_power();
        }

        void update_power()
        {
            uint16_t output_power = _power_level_control->get_output_power_level(_channel_id);
            if (output_power != _output_power)
            {
                _output_power = output_power;
                set_absolute_power(_output_power);
            }
        }

        virtual void loop(uint64_t time_us) = 0;
        virtual CChannel_types::channel_type get_channel_type() = 0;

    private:
        CPowerLevelControl *_power_level_control = NULL;        
        uint8_t _channel_id;
        CSavedSettings *_saved_settings = NULL;
        uint16_t _output_power = 0;
        uint32_t _led_colour = 0xFFFFFFFF;

    protected:
        virtual void set_absolute_power(uint16_t power) = 0;
        uint16_t _routine_power_level_requested = 0;

        uint32_t _standby_led_colour = LedColour::Black;

        void set_led_colour(uint32_t colour)
        {
            if (_channel_id >= 4)
                return;

            _led_colour = colour;

            uint8_t blue  =  colour        & 0xFF;
            uint8_t green = (colour >>  8) & 0xFF;
            uint8_t red   = (colour >> 16) & 0xFF;

            message msg = {0};
            msg.msg8[0] = MESSAGE_SET_LED_CHAN0 + _channel_id;
            msg.msg8[1] = red;
            msg.msg8[2] = green;
            msg.msg8[3] = blue;

            if (multicore_fifo_wready())
            {
                // printf("core1: LED %d push %d\t%d\t%d\n", _channel_id, red, green, blue);
                multicore_fifo_push_blocking(msg.msg32);
            }
            else
            {
                printf("COutputChannel::set_led_colour(): FIFO full\n");
            }
        }
};

#endif
