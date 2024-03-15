#ifndef _CROUTINE_H
#define _CROUTINE_H

#include "../../globals.h"
#include "../../AudioInput/AudioTypes.h"
#include "../output/CSimpleOutputChannel.h"
#include "../output/CFullOutputChannel.h"
#include "CAccPort.h"

#include <vector>
#include <inttypes.h>
#include <string>
#include <string.h>
#include <stdarg.h>

enum class output_type
{
    NONE,
    SIMPLE,
    FULL
};

enum class menu_entry_type
{
    MULTI_CHOICE,
    MIN_MAX,
    AUDIO_VIEW_SPECT,
    AUDIO_VIEW_WAVE,
    AUDIO_VIEW_INTENSITY_STEREO,
    AUDIO_VIEW_INTENSITY_MONO,
    AUDIO_VIEW_VIRTUAL_3
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

struct audio_view
{
    uint8_t default_trigger_position; // percent
};

struct menu_entry
{
    uint8_t id;
    uint8_t group_id; // Only relevant for display purposes when running patterns from the python GUI
    menu_entry_type menu_type;
    std::string title;
    struct min_max minmax;
    struct multi_choice multichoice;
    struct audio_view audioview;
};

struct routine_conf
{
    std::string name;
    std::vector<output_type> outputs;
    std::vector<menu_entry> menu;
    std::string button_text[(int)soft_button::BUTTON_MAX];
    bool enable_channel_isolation = true;
    audio_mode_t audio_processing_mode = audio_mode_t::OFF;
    uint16_t loop_freq_hz = 0;
    bool bluetooth_remote_passthrough = false; // If true, pass bt remote keypresses (e.g. "KEY_LEFT") straight through, without mapping to an action
};

class CRoutine;
typedef CRoutine* (*routine_creator)(uint8_t);

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
        virtual void menu_selected(uint8_t menu_id) {};
        virtual void trigger(trigger_socket socket, trigger_part part, bool active) {};
        virtual void soft_button_pushed (soft_button button, bool pushed) {}; // pushed: true=pushed, false=released
        virtual void bluetooth_remote_keypress(CBluetoothRemote::keypress_t key) {};

        virtual void audio_threshold_reached(uint16_t fundamental_freq, uint8_t cross_count) {};
        virtual void audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan) {};
        virtual void pulse_message(uint8_t channel, uint8_t pos_pulse_us, uint8_t neg_pulse_us) {};

        virtual lua_script_state_t lua_script_state()
        {
            return lua_script_state_t::NOT_APPLICABLE;
        }

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
            conf.audio_processing_mode = audio_mode_t::OFF;
            conf.enable_channel_isolation = true;

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

        static struct menu_entry new_menu_entry()
        {
            struct menu_entry entry;
            entry.id = 0;
            entry.group_id = 0;
            entry.title = "NOT SET";

            return entry;
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
                {
                    _full_channel[channel]->off();
                    _full_channel[channel]->set_freq(DEFAULT_FREQ_HZ);
                    _full_channel[channel]->set_pulse_width(DEFAULT_PULSE_WIDTH, DEFAULT_PULSE_WIDTH);
                }
            }
        }

        void print(text_type_t text_type, const char *format, ...)
        {
            pattern_text_output_t text_message;
            memset(&text_message, 0, sizeof(text_message));

            va_list args;
            va_start(args, format);

            text_message.text_type = text_type;
            text_message.time_generated_us = time_us_64();

            vsnprintf(text_message.text, sizeof(text_message.text)-1, format, args);
            text_message.text[sizeof(text_message.text)-1] = '\0';

            if (!queue_try_add(&gPatternTextOutputQueue, &text_message))
            {
                printf("gPatternTextOutputQueue FIFO was full\n");
            }

            va_end(args);
        }

    private:
        CFullOutputChannel *_full_channel[MAX_CHANNELS] = {0};
        CSimpleOutputChannel *_simple_channel[MAX_CHANNELS] = {0};
        bool _started = false;
};

#endif
