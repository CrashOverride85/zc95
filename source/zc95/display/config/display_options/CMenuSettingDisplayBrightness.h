#ifndef _CMENUSETTINGDISPLAYBRIGHTNESSS_H
#define _CMENUSETTINGDISPLAYBRIGHTNESSS_H

#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../CHorzBarGraph.h"
#include "../../../CSavedSettings.h"
#include "../../../core1/output/COutputChannel.h"
#include "../../../core1/routines/CRoutine.h"

class CMenuSettingDisplayBrightness : public CMenu
{
    public:
        CMenuSettingDisplayBrightness(CDisplay* display, CSavedSettings *saved_settings);
        ~CMenuSettingDisplayBrightness();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        CHorzBarGraph *_bar_graph = NULL;
        struct display_area _bar_graph_area;
        CDisplay* _display;
        CSavedSettings *_saved_settings;
        uint8_t _display_brightness=1;
};

#endif
