#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"
#include "../Bluetooth/CBluetooth.h"
#include "../Bluetooth/CBluetoothRemote.h"

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

     //   void show_selected_setting();

      /*  enum bt_keypress_t
        {
            ENABLED     = 0,
            SCAN        = 1,
            TEST        = 2
        }; */

        std::vector<setting_t> _settings;
        COptionsList *_keypress_list = NULL;

        std::vector<setting_t> _setting_choices;
        COptionsList *_keypress_action_list = NULL;
        
        void set_options_on_multi_choice_list(uint8_t setting_id);
        void save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index);
        setting_t get_current_setting();

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
        CBluetooth *_bluetooth;
};
