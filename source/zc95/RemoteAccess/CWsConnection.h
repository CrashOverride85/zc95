#ifndef _CWSCONNECTION_H
#define _CWSCONNECTION_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "httpd.h"

class CWsConnection
{
    public:
        CWsConnection(struct tcp_pcb *pcb);
        ~CWsConnection();
        void callback(uint8_t *data, u16_t data_len, uint8_t mode);
        void loop();
        bool active();

    private:
        enum class state_t
        {
            ACTIVE,
            DEAD
        };

        struct tcp_pcb *_pcb;
        state_t _state = state_t::ACTIVE;
};

#endif
