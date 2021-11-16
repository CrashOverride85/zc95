#ifndef _CMENUSETTINGCOLLARCONFIG_H
#define _CMENUSETTINGCOLLARCONFIG_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"

class CMenuCollarConfig : public CMenu
{
    public:
        CMenuCollarConfig(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output);
        ~CMenuCollarConfig();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        std::string get_collar_text(uint8_t collar_id);
        struct display_area _bar_graph_area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        uint8_t _led_brightness=1;
        COptionsList *_collar_list = NULL;
        uint8_t _selected_item;
        CRoutineOutput *_routine_output;
};



#endif
