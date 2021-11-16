#ifndef _CMENUSETTINGCOLLARCONFIGSELECTED_H
#define _CMENUSETTINGCOLLARCONFIGSELECTED_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"

class CMenuCollarConfigSelected : public CMenu
{
    public:
        CMenuCollarConfigSelected(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, uint8_t collar_id, CRoutineOutput *routine_output);
        ~CMenuCollarConfigSelected();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        enum setting_id
        {
            COLLAR_RF_ID   = 0,
            COLLAR_CHANNEL = 1,
            COLLAR_MODE    = 2,
            COLLAR_TEST    = 3
        };

        void set_button_a_text();
        void button_a_pressed();
        std::string get_collar_rf_id();
        void collar_test();

        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        uint8_t _led_brightness=1;
        COptionsList *_options_list = NULL;
        uint8_t _collar_id;
        CSavedSettings::collar_config _collar_conf;
        uint8_t _selected_item;
        CRoutineOutput *_routine_output;
};



#endif
