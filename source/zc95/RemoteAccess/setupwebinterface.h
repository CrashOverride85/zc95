/*
 * Author: Floris Bos
 * License: public domain / unlicense
 */

#pragma once

#include "../CSavedSettings.h"
#include "wlanscanner.h"
#include <string>

extern "C" 
{
    #include "RemoteAccess/dhcpserver.h"
    #include "RemoteAccess/dnserver.h"
}

class SetupWebInterface
{
    public:
        SetupWebInterface(CSavedSettings *saved_settings);
        virtual ~SetupWebInterface();
        bool alreadyConfigured();
        std::string savedSSID();        
        void startAccessPoint();
        void eraseSavedWlan();
        static void saveSettings(const std::string &ssid, const std::string &psk);
        std::string getQrCode();
        std::string getApSsid();
        std::string getApPsk();

    protected:
        std::string _ssid, _qr_code;

    private:
        ip4_addr_t _gw, _mask;
        dhcp_server_t _dhcp_server;
        CSavedSettings *_saved_settings;
        std::string _ap_ssid;
        bool _ap_started = false;
};
