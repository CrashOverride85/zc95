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

/* 
 * Handle an incoming websocket connection that's been accepted. Note that the websocket implementation 
 * in the LWIP HTTP server has something like an 8 second inactivity timeout before closing the connection, 
 * so the client needs to send _something_ every few seconds (the websocket protocol ping is good).
 * 
 * Processing of incoming messages is handled by CMessageProcessor.
 */

#include <ArduinoJson.h>
#include <list>
#include "CWsConnection.h"
#include "../git_version.h"

CWsConnection::CWsConnection(struct tcp_pcb *pcb, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines) 
{
    printf("CWsConnection::CWsConnection()\n");
    _pcb = pcb;
    _state = state_t::ACTIVE;
    _messageProcessor = new CMessageProcessor(routine_output, routines, std::bind(&CWsConnection::send, this, std::placeholders::_1));
}

CWsConnection::~CWsConnection()
{
    printf("~CWsConnection()\n");
    
    if (_messageProcessor != NULL)
    {
        delete _messageProcessor;
        _messageProcessor = NULL;
    }
}

// Process web socket message. This is called from the wifi/tcp polling thread
void CWsConnection::callback(uint8_t *data, u16_t data_len, uint8_t mode)
{
    printf("CWsConnection::callback()\n");

    // message isn't processed until _messageProcessor.loop() is called
    _messageProcessor->message(data, data_len);
}

void CWsConnection::loop()
{
    if (_state == state_t::DEAD)
        return;

    if (_pcb->state != ESTABLISHED)
    {
        printf("CWsConnection::loop(): connection closed\n");
        _state = state_t::DEAD;
        return;
    }

    _messageProcessor->loop();
}

void CWsConnection::send(std::string message)
{
    websocket_write(_pcb, (const uint8_t*)message.c_str(), message.length(), 0x01);
    
    tcp_output(_pcb);   // Send contents of TCP buffer now. Not required (will get sent 
                        // eventually without), but things are much faster with this.
}

bool CWsConnection::active()
{
    return (_state != state_t::DEAD);
}
