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

/* Init/Deinit wifi, get connection status, start web server, and do
 * wifi processing when loop() called.
 */

#include "CWifi.h"

#include "pico/cyw43_arch.h"
#include <stdio.h>
#include "pico/stdlib.h"

CWifi::CWifi(CRadio *radio, CAnalogueCapture *analogueCapture, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines)  : _routines(routines)
{
    printf("CWifi()\n");
    _analogue_capture = analogueCapture;
    _routine_output = routine_output;
    _radio = radio;
}


void CWifi::loop()
{
    if (!_wifi_init)
        return;

    if (_web_server)
    {
        _web_server->loop();
        cyw43_arch_poll();
    }
}

// returns true if connected with an IP, false otherwise
bool CWifi::get_connection_status(std::string &out_status_text)
{
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    switch (status)
    {
        case CYW43_LINK_DOWN:
            out_status_text = "Link down";
            break;

        case CYW43_LINK_JOIN:
            out_status_text = "Connecting"; // Don't want to say connected to wifi, as that sounds like all is good (no IP yet)
            break;

        case CYW43_LINK_NOIP:
            out_status_text = "Connected, no IP";
            break;

        case CYW43_LINK_UP:
            out_status_text = "Connected.";
            return true;

        case CYW43_LINK_FAIL:
            out_status_text = "Connection failed";
            break;

        case CYW43_LINK_NONET:
            out_status_text = "SSID not found";
            break;

        case CYW43_LINK_BADAUTH:
            out_status_text = "Auth fail";
            break;

        default:
            out_status_text = "(Unknown)";
            break;
    }

    return false;
}

void CWifi::start_ap()
{
    if (!init())
        return;

    cyw43_arch_enable_sta_mode();
    cyw43_wifi_pm(&cyw43_state, 0xa11140);
}

void CWifi::stop()
{
    stop_webserver();
    if (_wifi_init)
    {
        _radio->wifi(false);
        _wifi_init = false;
    }
}

bool CWifi::init()
{
    printf("CWifi::init()\n");
    if (_wifi_init)
    {
        return true;
    }

    if (_radio->wifi(true))
    {
        _wifi_init = true;
        return true;
    }
    else
    {
        return false;
    }
}

void CWifi::connect_to_wifi(std::string ssid, std::string psk)
{
    if (!init())
        return;

    // Without this, get:
    //   [CYW43] core not up
    //   [CYW43] core not up
    //   [CYW43] HT not ready
    _analogue_capture->stop();

    cyw43_arch_enable_sta_mode();
    cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 200, 1, 1, 10));
    int retval = cyw43_arch_wifi_connect_async(ssid.c_str(), psk.c_str(), CYW43_AUTH_WPA2_MIXED_PSK);
    if (retval)
    {
        printf("CWifi::connect_to_wifi(): error: cyw43_arch_wifi_connect_async returned %d\n", retval);
    }
    _analogue_capture->start();
}

void CWifi::start_webserver()
{
    if (!_web_server)
    {
        _web_server = new CWebServer(_routine_output, &_routines);
        _web_server->start();
    }
}

void CWifi::stop_webserver()
{
    if (_web_server)
    {
        delete _web_server;
        _web_server = NULL;
    }
}
