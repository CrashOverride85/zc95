#ifndef _CMENUSETTINGLEDBRIGHTNESS_H
#define _CMENUSETTINGLEDBRIGHTNESS_H

#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../CHorzBarGraph.h"
#include "../../../CSavedSettings.h"
#include "../../../core1/output/COutputChannel.h"
#include "../../../core1/routines/CRoutine.h"

class CMenuSettingLedBrightnes : public CMenu
{
    public:
        CMenuSettingLedBrightnes(CDisplay* display, CSavedSettings *saved_settings);
        ~CMenuSettingLedBrightnes();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        CHorzBarGraph *_bar_graph = NULL;
        struct display_area _bar_graph_area;
        CDisplay* _display;
        CSavedSettings *_saved_settings;
        uint8_t _led_brightness=1;
};






#endif
