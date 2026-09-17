#ifndef _CMenuSettingsBatteryReset_H
#define _CMenuSettingsBatteryReset_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "CMenuSettingsBatteryReset.h"
#include "../../../../Hal/IHal.h"

class CMenuSettingsBatteryReset : public CMenu
{
    public:
        CMenuSettingsBatteryReset(CDisplay* display, IHal* hal);
        ~CMenuSettingsBatteryReset();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:      
        CDisplay* _display;
        IHal *_hal;
        struct display_area _disp_area;
};

#endif
