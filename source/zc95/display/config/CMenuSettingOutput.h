#include "../CMenu.h"
#include "../CDisplay.h"
#include "../COptionsList.h"
#include "../CSavedSettings.h"
#include "../CChannel_types.h"
#include "../CHorzBarGraph.h"

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
        enum setting_id_t
        {
            POWER_LEVEL          = 0,
            RAMP_TIME            = 1
        };

        enum setting_kind_t
        {
            MULTI_CHOICE        = 0,
            MIN_MAX             = 1
        };

        setting_kind_t get_setting_kind(setting_id_t setting_id);
        setting_id_t get_currently_selected_setting_id();

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

        std::vector<setting_t> _settings;
        COptionsList *_settings_list = NULL;

        std::vector<setting_t> _setting_choices;
        COptionsList *_settings_choice_list = NULL;
        
        void set_options_for_setting(setting_id_t setting_id);
        void save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index);

        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
        CHorzBarGraph *_bar_graph = NULL;
        int16_t _min_max_value = 0;
        int16_t _min_max_value_min = 0;
        int16_t _min_max_value_max = 0;
};
