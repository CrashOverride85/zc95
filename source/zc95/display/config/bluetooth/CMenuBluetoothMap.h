#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../core1/CRoutineOutput.h"
#include "../../../Bluetooth/CBluetooth.h"
#include "../../../Bluetooth/CBluetoothRemote.h"

class CMenuBluetoothMap : public CMenu
{
    public:
        CMenuBluetoothMap(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth);
        ~CMenuBluetoothMap();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        class setting_t
        {
            public:
                setting_t(int id, std::string text)
                {
                    this->id = id;
                    this->text = text;
                }

                int id;
                std::string text;
        };

        std::vector<setting_t> _remote_keypress;
        COptionsList *_keypress_list = NULL;

        std::vector<setting_t> _keypress_action;
        COptionsList *_keypress_action_list = NULL;
        
        void set_actions_on_bottom_list(CBluetoothRemote::keypress_action_t current_action);

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
        CBluetooth *_bluetooth;
};
