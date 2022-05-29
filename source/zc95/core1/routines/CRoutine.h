#ifndef _CROUTINES_H
#define _CROUTINES_H


#include "../output/CSimpleOutputChannel.h"
#include "../output/CFullOutputChannel.h"
#include "CAccPort.h"

#include <vector>
#include <inttypes.h>
#include <string>

enum class output_type
{
    NONE,
    SIMPLE,
    FULL
};

enum class menu_entry_type
{
    MULTI_CHOICE,
    MIN_MAX
};

enum class trigger_socket
{
    Trigger1,
    Trigger2
};

enum class trigger_part
{
    A,
    B
};

enum class soft_button
{
    BUTTON_A,
    BUTTON_MAX
};


#define POWER_FULL 1000

struct min_max
{
    std::string UoM; // Unit of Measure (e.g. "s" for seconds). Leave blank if not applicable
    uint16_t increment_step;
    uint16_t min;
    uint16_t max;
    uint16_t current_value;
};

struct multi_choice_option
{
    std::string choice_name;
    uint8_t choice_id;
};

struct multi_choice
{
    std::vector<multi_choice_option> choices;
    uint8_t current_selection;
};

struct menu_entry
{
    uint8_t id;
    menu_entry_type menu_type;
    std::string title;
    struct min_max minmax;
    struct multi_choice multichoice;
};

struct routine_conf
{
    std::string name;
    std::vector<output_type> outputs;
    std::vector<menu_entry> menu;
    std::string button_text[(int)soft_button::BUTTON_MAX];
};


class CRoutine
{
    public:
        virtual ~CRoutine() 
        {
            if (_started)
            {
                acc_port.reset();
                set_all_channels_off();
            }
        };

        virtual void get_config(struct routine_conf *conf) = 0;
        virtual void start() = 0;
        virtual void menu_min_max_change(uint8_t menu_id, int16_t new_value) {};
        virtual void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id) {};
        virtual void trigger(trigger_socket socket, trigger_part part, bool active) {};
        virtual void soft_button_pushed (soft_button button, bool pushed) {}; // pushed: true=pushed, false=released

        virtual void loop(uint64_t time_us) = 0;
        virtual void stop() = 0;

        void set_simple_output_channel(uint8_t channel, CSimpleOutputChannel *simple_output_channel)
        {
            _started = true;
            if (channel < MAX_CHANNELS)
                _simple_channel[channel] = simple_output_channel;
        };
        
        void set_full_output_channel(uint8_t channel, CFullOutputChannel *full_output_channel) 
        {
            _started = true;
            if (channel < MAX_CHANNELS)
                _full_channel[channel] = full_output_channel;
        };       

        static void reset_routine_conf(struct routine_conf &conf)
        {   
            conf.name = "";
            conf.outputs.clear();
            conf.menu.clear();

            for (int x=0; x < (int)soft_button::BUTTON_MAX; x++)
                conf.button_text[x] = " ";
        }

    protected:
        CAccPort acc_port;
        static struct multi_choice_option get_choice(std::string choice_name, uint8_t choice_id)
        {
             struct multi_choice_option  choice;
             choice.choice_id = choice_id;
             choice.choice_name = choice_name;
             return choice;
        }

        void simple_channel_set_power(uint8_t channel, uint16_t power)
        {
            if (channel < MAX_CHANNELS && _simple_channel[channel] != NULL)
            {
                _simple_channel[channel]->channel_set_power(power);
            }
        }

        void simple_channel_pulse(uint8_t channel, uint16_t min_pulse_ms)
        {
            if (channel < MAX_CHANNELS && _simple_channel[channel] != NULL)
            {
                _simple_channel[channel]->channel_pulse(min_pulse_ms);
            }
        }

        void simple_channel_on(uint8_t channel)
        {
            if (channel < MAX_CHANNELS && _simple_channel[channel] != NULL)
            {
                _simple_channel[channel]->channel_on();
            }
        }

        void simple_channel_off(uint8_t channel)
        {
            if (channel < MAX_CHANNELS && _simple_channel[channel] != NULL)
            {
                _simple_channel[channel]->channel_off();
            }
        }

        void full_channel_pulse(uint8_t channel, uint16_t pos_us, uint16_t neg_us)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->channel_pulse(pos_us, neg_us);
            }
        }

        void full_channel_set_power(uint8_t channel, uint16_t power)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->channel_set_power(power);
            }
        }

        void full_channel_set_freq(uint8_t channel, uint16_t freq)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->set_freq(freq);
            }
        }

        void full_channel_set_pulse_width(uint8_t channel, uint8_t pos_us, uint8_t neg_us)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->set_pulse_width(pos_us, neg_us);
            }
        }

        void full_channel_on(uint8_t channel)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->on();
            }
        }

        void full_channel_off(uint8_t channel)
        {
            if (channel < MAX_CHANNELS && _full_channel[channel] != NULL)
            {
                _full_channel[channel]->off();
            }
        }

        void set_all_channels_power(uint16_t power)
        {
            for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
            {
                if (_full_channel[channel] != NULL)
                    _full_channel[channel]->channel_set_power(power);

                if (_simple_channel[channel] != NULL)
                    _simple_channel[channel]->channel_set_power(power);
            }
        }

        void set_all_channels_off()
        {
            for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
            {
                if (_simple_channel[channel] != NULL)
                    _simple_channel[channel]->channel_off();

                if (_full_channel[channel] != NULL)
                    _full_channel[channel]->off();
            }
        }

    private:
        CFullOutputChannel *_full_channel[MAX_CHANNELS] = {0};
        CSimpleOutputChannel *_simple_channel[MAX_CHANNELS] = {0};
        bool _started = false;
};

#endif