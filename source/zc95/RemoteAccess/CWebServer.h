#ifndef _CWEBSERVER_H
#define _CWEBSERVER_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <bits/stdc++.h>
#include "httpd.h"
#include "CWsConnection.h"
#include "../CAnalogueCapture.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CWebServer
{
    public:
        CWebServer(CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines);
        ~CWebServer();
        void start();
        void loop();

    private:
        static const char *websocket_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
        static void websocket_open_cb(struct tcp_pcb *pcb, const char *uri);
        static void websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode);

        bool _started = false;
        static std::vector<CRoutines::Routine> *_s_routines;
        static std::map<struct tcp_pcb*, CWsConnection*> _s_ws_connections;
        static CAnalogueCapture *_s_analogue_capture;
        static CRoutineOutput *_s_routine_output;
};

#endif
