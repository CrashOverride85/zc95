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
        void loop();

    private:
        std::function<void(std::string)> _send;
        std::function<void(std::string result, int msg_count, std::string error)> _send_ack;
        void send_power_status_update();
        void send_lua_script_error_message();
        void send_ack(std::string result, int msg_count);
        void script_output(pattern_text_output_t output);
        
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine>& _routines;
        uint16_t _output_power[MAX_CHANNELS];
        uint16_t _max_output_power[MAX_CHANNELS];
        uint16_t _front_panel_power[MAX_CHANNELS]; // acts as power limit in remote access mode
        uint64_t _last_power_status_update_us = 0;
        lua_script_state_t _lua_script_state = lua_script_state_t::NOT_APPLICABLE;
};

#endif
