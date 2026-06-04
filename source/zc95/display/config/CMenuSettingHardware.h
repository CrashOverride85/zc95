#include "../CMenu.h"
#include "../CDisplay.h"
#include "../COptionsList.h"
#include "../CSavedSettings.h"
#include "../CChannel_types.h"
#include "../AudioInput/CAudio.h"
#include "../Hal/IHal.h"
#include "../core1/output/COutputChannel.h"
#include "../core1/routines/CRoutine.h"
#include "../core1/CRoutineOutput.h"
#include "../Hal/IHal.h"
#include "../CHorzBarGraph.h"

class CMenuSettingHardware : public CMenu
{
    public:
        CMenuSettingHardware(CDisplay* display, CSavedSettings *saved_settings, CRoutineOutput *routine_output, CAudio *audio, IHal* hal);
        ~CMenuSettingHardware();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        void show_selected_setting();

        enum setting_id
        {
            AUDIO          = 0,
            DEBUG          = 1,
            AUX_USE        = 2,
            CHARGE_CURRENT = 3,
            LED_FORMAT     = 4
        };

        COptionsList *_settings_list = NULL;
        COptionsList *_settings_choice_list = NULL;
        
        void set_options_for_selection(uint8_t setting_id);
        void save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index);

        struct display_area _area;
        CDisplay* _display;
        IHal *_hal;
        display_area _setting_choice_area;
        CSavedSettings *_saved_settings;
        CRoutineOutput *_routine_output;
        CAudio *_audio;
        CHorzBarGraph *_bar_graph = NULL;
        int16_t _charge_current_ma = 0;
};
