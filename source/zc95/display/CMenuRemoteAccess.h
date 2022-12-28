#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "CMenuRemoteAccessConnectWifi.h"
#include "../CSavedSettings.h"
#include "../CAnalogueCapture.h"
#include "../RemoteAccess/CWifi.h"

class CMenuRemoteAccess : public CMenu
{
    public:
        CMenuRemoteAccess(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CWifi *wifi, CAnalogueCapture *analogueCapture);
        ~CMenuRemoteAccess();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        class option
        {
            public:
                option(int id, std::string text)
                {
                    this->id = id;
                    this->text = text;
                }

                int id;
                std::string text;
        };

        void show_selected_setting();

        enum option_id
        {
            AP_MODE      = 0,
            CONNECT_WIFI = 1
        };

        std::vector<option> _options;
        int _last_selection = -1;
        COptionsList *_options_list = NULL;
        struct display_area _area;
        
        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CWifi *_wifi;
        CAnalogueCapture *_analogueCapture;
};




