#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/output/COutputChannel.h"
#include "../core1/output/CChannelConfig.h"
#include "../core1/routines/CRoutine.h"

class CMenuSettings : public CMenu
{
    public:
        CMenuSettings(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output);
        ~CMenuSettings();
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
            CHANNEL_CONFIG = 0,
            COLLAR_CONFIG  = 1,
            LED_BRIGHTNESS = 2,
            POWER_STEP     = 3,
            RAMP_UP_TIME   = 4,
            ABOUT          = 5
        };

        std::vector<setting> _settings;


        COptionsList *_settings_list = NULL;
        struct display_area _area;
        CDisplay* _display;

        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CRoutineOutput *_routine_output;
};




