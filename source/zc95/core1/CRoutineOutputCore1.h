#ifndef _CROUTINEOUTPUTCORE1_H
#define _CROUTINEOUTPUTCORE1_H

#include <functional>
#include "CRoutineOutput.h"
#include "Core1.h"
#include "../display/CDisplay.h"
#include "../CLedControl.h"
#include "../CExtInputPortExp.h"
#include "../EExtInputPort.h"


class CRoutineOutputCore1 : public CRoutineOutput
{
    public:
        CRoutineOutputCore1(CDisplay *display, CLedControl *led_control, CExtInputPortExp **ext_port_exp);
        void set_front_panel_power(uint8_t channel, uint16_t power);
        void set_remote_power(uint8_t channel, uint16_t power);
        void enable_remote_power_mode();
        void disable_remote_power_mode();
        uint16_t get_output_power(uint8_t channel);
        uint16_t get_front_pannel_power(uint8_t channel);
        uint16_t get_max_output_power(uint8_t channel);
        
        void activate_routine(uint8_t routine_id);
        void stop_routine();

        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void menu_selected(uint8_t menu_id);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void soft_button_pressed(soft_button button, bool pressed);
        void loop();

        void collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power);
        void reinit_channels();
        void suspend_core1();
        void audio_threshold_reached(uint16_t fundamental_freq, uint8_t cross_count);
        void audio_intensity_change(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan = 0);

        void reset_acc_port();
        void set_acc_io_port_state(enum ExtInputPort output, bool high);
        lua_script_state_t get_lua_script_state();
        void set_text_callback_function(std::function<void(pattern_text_output_t)> cb);

    private:
        union __attribute__((packed)) message
        {
            uint32_t msg32;
            uint8_t msg8[4];
        };

        void update_display(uint8_t channel);
        void process_message(message msg);
        void process_text_message_queue();

        Core1 *_core1 = NULL;
        CDisplay *_display;
        CLedControl *_led_control;
        CExtInputPortExp **_ext_port_exp;
        uint16_t _front_pannel_power[MAX_CHANNELS] = {0};
        uint16_t _remote_power[MAX_CHANNELS] = {0};
        uint16_t _output_power[MAX_CHANNELS] = {0};
        uint16_t _max_output_power[MAX_CHANNELS] = {0};      
        bool _remote_mode_active = false;
        lua_script_state_t _lua_script_state = lua_script_state_t::NOT_APPLICABLE;
        std::function<void(pattern_text_output_t)> _text_output_callback = NULL;
};

#endif
