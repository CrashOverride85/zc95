#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../CChannel_types.h"
#include "../../../core1/output/COutputChannel.h"
#include "../../../core1/routines/CRoutine.h"
#include "../../../core1/CRoutineOutput.h"

class CMenuChannelConfig : public CMenu
{
    public:
        CMenuChannelConfig(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output);
        ~CMenuChannelConfig();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        struct channel_choice
        {
            CChannel_types::channel_type type;
            uint8_t channel_index;
            std::string text;
        };

        void set_options_on_multi_choice_list(uint8_t channel_id);
        void save_selected_channel(uint8_t channel_id, channel_choice choice);

        std::vector<channel_choice> _channel_choices;
        COptionsList *_channel_list = NULL;
        COptionsList *_channel_choices_list = NULL;
        struct display_area _area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        display_area _channel_choice_area;
        CSavedSettings *_saved_settings;
        channel_choice get_channel_choice(CChannel_types::channel_type, uint8_t index);
        CRoutineOutput *_routine_output;
};




