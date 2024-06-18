#ifndef _CWSCONNECTION_H
#define _CWSCONNECTION_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "httpd.h"
#include "CLuaLoad.h"
#include "CRoutineRun.h"
#include "CMessageProcessor.h"
#include "../config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CWsConnection
{
    public:
        CWsConnection(struct tcp_pcb *pcb, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines);
        ~CWsConnection();
        void callback(uint8_t *data, u16_t data_len, uint8_t mode);
        void send(std::string message);
        void loop();
        bool active();

    private:
        enum class state_t
        {
            ACTIVE,
            DEAD
        };

        CMessageProcessor *_messageProcessor;
        struct tcp_pcb *_pcb;
        state_t _state = state_t::ACTIVE;
};

#endif
