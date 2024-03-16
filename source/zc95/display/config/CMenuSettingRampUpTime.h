#ifndef _CMENUSETTINGRAMPUPTIME_H
#define _CMENUSETTINGRAMPUPTIME_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "../COptionsList.h"
#include "../CHorzBarGraph.h"
#include "../CSavedSettings.h"
#include "../core1/output/COutputChannel.h"
#include "../core1/routines/CRoutine.h"

class CMenuSettingRampUpTime : public CMenu
{
    public:
        CMenuSettingRampUpTime(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings);
        ~CMenuSettingRampUpTime();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        CHorzBarGraph *_bar_graph = NULL;
        struct display_area _bar_graph_area;
        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        uint8_t _ramp_up_time_seconds = 5;
};






#endif
