#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "CMenuRemoteAccessConnectWifi.h"
#include "../../../CSavedSettings.h"
#include "../../../CAnalogueCapture.h"
#include "../../../RemoteAccess/CWifi.h"
#include "../../../core1/CRoutineOutput.h"
#include "../../../Bluetooth/CBluetooth.h"

class CMenuRemoteAccess : public CMenu
{
    public:
        CMenuRemoteAccess(
            CDisplay* display,
            CGetButtonState *buttons, 
            CSavedSettings *saved_settings, 
            CWifi *wifi, 
            CAnalogueCapture *analogueCapture, 
            CRoutineOutput *routine_output,
            std::vector<CRoutines::Routine> &routines,
            CBluetooth *bluetooth,
            CRadio *radio);

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
        std::string get_serial_config_error();

        enum option_id
        {
            AP_MODE             = 0,
            CONNECT_WIFI        = 1,
            CLEAR_SAVED_CREDS   = 2,
            REGEN_AP_PSK        = 3,
            SERIAL_ACCESS       = 4,
            BLE_GATT            = 5,
            BLE_CONFIG          = 6
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
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine>& _routines;
        CBluetooth *_bluetooth;
        CRadio *_radio;
};
