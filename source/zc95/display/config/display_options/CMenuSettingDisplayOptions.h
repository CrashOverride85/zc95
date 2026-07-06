#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../Hal/IHal.h"

class CMenuSettingDisplayOptions : public CMenu
{
    public:
        CMenuSettingDisplayOptions(
            CDisplay* display, 
            CSavedSettings *saved_settings,
            IHal *hal);

        ~CMenuSettingDisplayOptions();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        class setting
        {
            public:
                setting(int id, std::string text)
                {
                    this->id = id;
                    this->text = text;
                }

                int id;
                std::string text;
        };

        void show_selected_setting();

        enum setting_id
        {
            LED_BRIGHTNESS      = 0,
            POWER_LEVEL_DISPLAY = 1,
            BUTTON_BRIGHTNESS   = 2,
            STATUS_BAR_TEXT     = 3,
            DISPLAY_BRIGHTNESS  = 4,
            BATTERY_STATUS      = 5
        };

        std::vector<setting> _settings;
        int _last_selection = -1;
        COptionsList *_settings_list = NULL;
        struct display_area _area;
        
        CDisplay* _display;
        CSavedSettings *_saved_settings;
        IHal *_hal;
};
