#include "../CMenu.h"
#include "../CDisplay.h"
#include "../COptionsList.h"
#include "../CSavedSettings.h"
#include "../CChannel_types.h"
#include "../AudioInput/CAudio.h"
#include "../core1/output/COutputChannel.h"
#include "../core1/routines/CRoutine.h"
#include "../core1/CRoutineOutput.h"

class CMenuSettingOutput : public CMenu
{
    public:
        CMenuSettingOutput(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings);
        ~CMenuSettingOutput();
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

        void show_selected_setting();

        enum setting_id
        {
            POWER_LEVEL          = 0
        };

        std::vector<setting_t> _settings;
        COptionsList *_settings_list = NULL;

        std::vector<setting_t> _setting_choices;
        COptionsList *_settings_choice_list = NULL;
        
        void set_options_on_multi_choice_list(uint8_t setting_id);
        void save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index);

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
};
