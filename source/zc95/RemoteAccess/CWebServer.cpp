/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/* Start/manage the LWIP HTTP webserver for the web socket interface when 
 * connected to WiFi. Doesn't deal with AP mode (yet).
 * Creates an instance of CWsConnection to deal with incoming web socket 
 * connections.
 *
 * WARNINGS:
 *  1. Uses statics for a few things, so creating mutiple instances of this 
 *     class is not going to go well
 *  2. Only one Websocket connection is permitted. This class should be fine 
 *     with more, but if two web socket connections are trying to control the
 *     box at once, that would be bad
*/

#include "CWebServer.h"
#include <queue>

std::map<struct tcp_pcb*, CWsConnection*> CWebServer::_s_ws_connections;
CRoutineOutput *CWebServer::_s_routine_output;
std::vector<CRoutines::Routine> *CWebServer::_s_routines;

CWebServer::CWebServer(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines)
{
    printf("CWebServer()\n");
    _s_routine_output = routine_output;
    _s_routines = routines;
}

CWebServer::~CWebServer()
{
    printf("~CWebServer()\n");

    std::map<struct tcp_pcb*, CWsConnection*>::iterator it;
    for (it = _s_ws_connections.begin(); it != _s_ws_connections.end(); it++)
    {
        if (it->second != NULL)
            delete it->second;
    }
    _s_ws_connections.clear();
}

void CWebServer::start()
{
    if (_started)
        return;
    
    printf("CWebServer::start(): starting http server\n");

    websocket_register_callbacks((tWsOpenHandler) websocket_open_cb, (tWsHandler) websocket_cb);
    httpd_init(0);
}

void CWebServer::loop()
{
    std::queue<struct tcp_pcb*> connections_to_remove;
    
    std::map<struct tcp_pcb*, CWsConnection*>::iterator it;
    for (it = _s_ws_connections.begin(); it != _s_ws_connections.end(); it++)
    {
        it->second->loop();

        // Clear up any closed connections
        if (!it->second->active())
        {
            connections_to_remove.push(it->first);
        }
    }

    while (!connections_to_remove.empty())
    {
        delete _s_ws_connections[connections_to_remove.front()];
        _s_ws_connections.erase(connections_to_remove.front());
        connections_to_remove.pop();
    }
}

void CWebServer::websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode)
{
    if (_s_ws_connections.count(pcb) != 1)
    {
        printf("websocket_cb: connection not found!\n");
        return;
    }

    _s_ws_connections[pcb]->callback(data, data_len, mode);
}

void CWebServer::websocket_open_cb(struct tcp_pcb *pcb, const char *uri)
{
    printf("WS URI: %s\n", uri);

    if (_s_ws_connections.size() >= MAX_WEBSOCKET_CONNECTIONS)
    {
        printf("CWebServer::websocket_open_cb(): incoming connection, but already at connection limit. Closing connection.\n");
        tcp_close(pcb);
        return;
    }

    if (!strcmp(uri, "/stream")) 
    {
        printf("request for streaming\n");
        _s_ws_connections[pcb] = new CWsConnection(pcb, _s_routine_output, _s_routines);
    }
}

const char *CWebServer::websocket_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    return "/websockets.html";
}
