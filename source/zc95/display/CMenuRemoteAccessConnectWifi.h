#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../CAnalogueCapture.h"
#include "../RemoteAccess/CWifi.h"

class CMenuRemoteAccessConnectWifi : public CMenu
{
    public:
        CMenuRemoteAccessConnectWifi(
            CDisplay* display,
            CGetButtonState *buttons, 
            CSavedSettings *saved_settings, 
            CWifi *wifi,
            CAnalogueCapture *analogueCapture);
        ~CMenuRemoteAccessConnectWifi();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:

        enum state_t
        {
            NOT_CONFIGURED = 0,
            CONNECTING     = 1,
            CONNECTED      = 2
        };

        struct display_area _area;
        
        CDisplay* _display;
        display_area _disp_area;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CWifi *_wifi;
        CAnalogueCapture *_analogue_capture;
        state_t _state = state_t::CONNECTING;
        std::string _ssid;
        uint64_t _last_connecting_screen_update_us = 0;  
        uint8_t _connecting_dot_count = 0;

};
