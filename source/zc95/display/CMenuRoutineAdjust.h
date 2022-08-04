#ifndef _CMENUROUTINEADJUST_H
#define _CMENUROUTINEADJUST_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"

#include "../core1/routines/CRoutine.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutineMaker.h"
#include "../config.h"
#include "../ECButtons.h"
#include "../CGetButtonState.h"
#include "../AudioInput/CAudio.h"

#include <string>
#include <vector>

class CMenuRoutineAdjust : public CMenu
{
    public:
        CMenuRoutineAdjust(CDisplay* display, CRoutineMaker* routine_maker, CGetButtonState *buttons, CRoutineOutput *routine_output, CAudio *audio);
        ~CMenuRoutineAdjust();
        void button_pressed(Button button);
        void button_released(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        void set_options_on_multi_choice_list();
        void draw_horz_bar_graph(int16_t x, int16_t y, uint8_t width, uint8_t height, int16_t min_val, int16_t max_val, int16_t current_val, std::string UoM, color_t bar_colour);
        void enable_audio_if_required_by_routine();

        COptionsList *_routine_adjust_display_list = NULL;
        COptionsList *_routine_multi_choice_list = NULL;
        struct display_area _area;
        CDisplay* _display;
        struct routine_conf _active_routine_conf;
        CGetButtonState *_buttons;
        CRoutineOutput *_routine_output;
        CAudio *_audio;
};

#endif
