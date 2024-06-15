#ifndef _CWIFI_H
#define _CWIFI_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "CWebServer.h"
#include "../CAnalogueCapture.h"
#include "../CRadio.h"
#include "../core1/routines/CRoutines.h"

class CWifi
{
    public:
        CWifi(CRadio *radio, CAnalogueCapture *analogueCapture, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines);
        void loop();

        void start_ap();
        void connect_to_wifi(std::string ssid, std::string psk);
        bool get_connection_status(std::string &out_status_text); // returns true if connected with an IP, false otherwise
        void stop();
        void start_webserver();
        void stop_webserver();

    private:
        bool init();
        bool _wifi_init = false;
        CRadio *_radio;
        CAnalogueCapture *_analogue_capture;
        CRoutineOutput *_routine_output;
        CWebServer *_web_server = NULL;
        std::vector<CRoutines::Routine>& _routines;
};

#endif
