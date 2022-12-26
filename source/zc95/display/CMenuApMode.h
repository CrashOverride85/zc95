#ifndef _CMENUAPMODE_H
#define _CMENUAPMODE_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../RemoteAccess/CWifi.h"
#include "../RemoteAccess/setupwebinterface.h"
#include "../CSavedSettings.h"
#include "../CAnalogueCapture.h"

class CMenuApMode : public CMenu
{
    public:
        CMenuApMode(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CWifi *wifi, CAnalogueCapture *analogueCapture);
        ~CMenuApMode();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

        void wifi_scan();

    private:
        enum state_t
        {
            INIT             = 0,
            WIFI_SCAN        = 1,
            AP_MODE_STARTED  = 2
        };

        void show_qr_code(std::string str);

        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CAnalogueCapture *_analogueCapture;
        uint8_t _selected_item;
        CWifi *_wifi;
        state_t _state = state_t::INIT;      
        uint64_t _scan_start_us;  
        SetupWebInterface *_setupwebinterface = NULL;
        std::string _qr_code;
};



#endif
