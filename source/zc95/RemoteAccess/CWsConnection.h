#ifndef _CWSCONNECTION_H
#define _CWSCONNECTION_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "httpd.h"
#include "CLuaLoad.h"

class CWsConnection
{
    public:
        CWsConnection(struct tcp_pcb *pcb);
        ~CWsConnection();
        void callback(uint8_t *data, u16_t data_len, uint8_t mode);
        void send(std::string message);
        void loop();
        bool active();

    private:
        enum class state_t
        {
            ACTIVE,
            LUA_LOAD,
            DEAD
        };

        void send_ack(std::string result, int msg_count);
        void set_state(state_t new_state);

        struct tcp_pcb *_pcb;
        state_t _state = state_t::ACTIVE;
        CLuaLoad *_lua_load = NULL;
};

#endif
