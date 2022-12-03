
#ifndef _CCORE1_H
#define _CCORE1_H


#include "routines/CRoutine.h"
#include "routines/CRoutines.h"

#include "output/CChannelConfig.h"
#include "output/collar/CCollarComms.h"

#include "output/CFullChannelAsSimpleChannel.h"

#include "../CSavedSettings.h"
#include "CPowerLevelControl.h"
#include "Core1Messages.h"

extern  mutex_t g_collar_message_mutex;
extern CCollarComms::collar_message g_collar_message;


class Core1
{

    public:
        Core1(std::vector<CRoutines::Routine> *routines, CSavedSettings *saved_settings);
        ~Core1();
        void init();
        void loop();
        void activate_routine(uint8_t routine_id);
        void stop_routine();
        CPowerLevelControl *power_level_control;
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void menu_selected(uint8_t menu_id);
        void update_channel_power(uint8_t channel);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power);
        void soft_button_pushed(soft_button button, bool pushed);

    private:
        void delete_fullChannelAsSimpleChannels_and_restore_channels();
        void process_messages();
        void process_message(message msg);
        void update_power_levels();
        void set_output_chanels_to_off(bool enable_channel_isolation);
        void process_audio_pulse_queue();

        CChannelConfig *_channel_config;
        CRoutine *_active_routine = NULL;
        COutputChannel* _active_channels[MAX_CHANNELS];
        CFullChannelAsSimpleChannel *_fullChannelAsSimpleChannels[MAX_CHANNELS];
        CSavedSettings *_saved_settings;
        COutputChannel *_real_output_channel[MAX_CHANNELS];
        std::vector<CRoutines::Routine> *_routines;
        uint16_t _output_power[MAX_CHANNELS] = {0};
        uint16_t _output_power_max[MAX_CHANNELS] = {0};    
        pulse_message_t _pulse_messages[MAX_CHANNELS] = {0};
};

Core1* core1_start(std::vector<CRoutines::Routine> *routines, CSavedSettings *saved_settings);

#endif

