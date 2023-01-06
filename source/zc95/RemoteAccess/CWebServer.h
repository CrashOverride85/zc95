#ifndef _CWEBSERVER_H
#define _CWEBSERVER_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <bits/stdc++.h>
#include "httpd.h"
#include "CWsConnection.h"

class CWebServer
{
    public:
        CWebServer();
        ~CWebServer();
        void start();
        void loop();
        

    private:
        static char *websocket_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
        static void websocket_open_cb(struct tcp_pcb *pcb, const char *uri);
        static void websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode);

        bool _started = false;
        static std::map<struct tcp_pcb*, CWsConnection*> _s_ws_connections;

};

#endif
