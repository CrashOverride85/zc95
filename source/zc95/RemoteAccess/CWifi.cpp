#include "CWifi.h"

#include "pico/cyw43_arch.h"
#include <stdio.h>
#include "pico/stdlib.h"

CWifi::CWifi(CAnalogueCapture *analogueCapture)
{
    _analogue_capture = analogueCapture;
}

void CWifi::loop()
{
    if (!_wifi_init)
        return;
    
    cyw43_arch_poll();

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
        printf("CWifi::stop(): Deinit wifi\n");
        cyw43_arch_deinit();
        _wifi_init = false;
    }
}

bool CWifi::init()
{
    if (_wifi_init)
    {
        return true;
    }

    if (cyw43_arch_init())
    {
        printf("CWifi::init(): Failed to init wifi/cyw43\n");
        return false;
    }
    else
    {
        printf("CWifi::init(): wifi/cyw43 init success\n");
        _wifi_init = true;
        return true;
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
    cyw43_wifi_pm(&cyw43_state, 0xa11140);
    cyw43_arch_wifi_connect_async(ssid.c_str(), psk.c_str(), CYW43_AUTH_WPA2_MIXED_PSK);
    _analogue_capture->start();
}

void CWifi::start_webserver()
{
    if (!_web_server)
    {
        _web_server = new CWebServer();
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
