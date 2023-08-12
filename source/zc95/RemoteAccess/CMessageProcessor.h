#ifndef _CMESSAGEPROCESSOR_H
#define _CMESSAGEPROCESSOR_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "httpd.h"
#include "CLuaLoad.h"
#include "CRoutineRun.h"
#include "../config.h"
#include "../CAnalogueCapture.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CMessageProcessor
{
    public:
        CMessageProcessor(
                CAnalogueCapture *analogue_capture, 
                CRoutineOutput *routine_output, 
                std::vector<CRoutines::Routine> *routines, 
                std::function<void(std::string)> send_function
        );
        ~CMessageProcessor();
        void message(uint8_t *data, u16_t data_len);
        void loop();

    private:
        enum class state_t
        {
            ACTIVE,
            LUA_LOAD,
            ROUTINE_RUN
        };

        void send_ack(std::string result, int msg_count, std::string error = "");
        void set_state(state_t new_state);
        void reload_routines();
        void send_lua_scripts(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        void delete_lua_script(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        void send_pattern_list(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        void send_pattern_detail(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        void send_version_details(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);

        std::function<void(std::string)> _send;

        CAnalogueCapture *_analogue_capture;
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine> *_routines;

        state_t _state = state_t::ACTIVE;
        CLuaLoad *_lua_load = NULL;
        CRoutineRun *_routine_run = NULL;

        char *_pending_message_buffer  = NULL;
        bool _pending_message = false;
};

#endif
