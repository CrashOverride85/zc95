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
            std::vector<CRoutines::Routine> *routines);
        ~CRoutineRun();
        bool process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);
        bool routines_updated();

    private:
        std::function<void(std::string)> _send;
        std::function<void(std::string result, int msg_count, std::string error)> _send_ack;
        void send_ack(std::string result, int msg_count);
        
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine> *_routines;

};

#endif
