#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"

class CMenuSettingPowerLevelDisplay : public CMenu
{
    public:
        CMenuSettingPowerLevelDisplay(CDisplay* display, CSavedSettings *saved_settings);
        ~CMenuSettingPowerLevelDisplay();
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

        std::vector<setting_t> _setting_choices;
        COptionsList *_settings_choice_list = NULL;
        
        void set_options_on_multi_choice_list();
        void save_setting(uint8_t choice_menu_index);

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
};
