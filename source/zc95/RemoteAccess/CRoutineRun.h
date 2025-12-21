#ifndef _CROUTINERUN_H
#define _CROUTINERUN_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <functional>
#include <ArduinoJson.h>
#include "../config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CRoutineRun
{
    public:
        CRoutineRun(
            std::function<void(std::string)> send_function, 
            std::function<void(std::string result, int msg_count, std::string error)> send_ack_func,
            CRoutineOutput *routine_output,
            std::vector<CRoutines::Routine> &routines);
        ~CRoutineRun();
        bool process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        bool StartPattern(ArduinoJson6200_71::StaticJsonDocument<300U> *doc, int msgId, bool &pattern_start, bool &retFlag);
        void loop();

    private:
        std::function<void(std::string)> _send;
        std::function<void(std::string result, int msg_count, std::string error)> _send_ack;
        void send_power_status_update();
        void send_menu_change_update(uint8_t menu_id, uint16_t value);
        void send_lua_script_error_message();
        void send_ack(std::string result, int msg_count);
        void script_output(pattern_text_output_t output);
        void set_pattern_config(uint8_t index);
        void menu_changed_callback(menu_change_msg_t msg);
        
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine>& _routines;
        uint16_t _output_power[MAX_CHANNELS];
        uint16_t _max_output_power[MAX_CHANNELS];
        uint16_t _front_panel_power[MAX_CHANNELS]; // acts as power limit in remote access mode
        uint64_t _last_power_status_update_us = 0;
        lua_script_state_t _lua_script_state = lua_script_state_t::NOT_APPLICABLE;
        struct routine_conf _pattern_conf;
        bool _running = false;
};

#endif
