#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"

class CMenuSettingBatteryStatus : public CMenu
{
    public:
        CMenuSettingBatteryStatus(CDisplay* display, CSavedSettings *saved_settings);
        ~CMenuSettingBatteryStatus();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        COptionsList *_settings_choice_list = NULL;

        void set_options_on_multi_choice_list();
        void save_setting();

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
};
