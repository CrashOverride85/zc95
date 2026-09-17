#ifndef _CMENUSETTINGBATTERYINFO_H
#define _CMENUSETTINGBATTERYINFO_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "../../Hal/IHal.h"
#include "../../PowerManagement/IPowerManagement.h"

class CMenuSettingBatteryInfo : public CMenu
{
    public:
        CMenuSettingBatteryInfo(CDisplay* display, IHal* hal);
        ~CMenuSettingBatteryInfo();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        void put_text_line(int16_t x, int16_t y, uint8_t line, hagl_color_t colour, std::string text);
        CDisplay* _display;
        IHal* _hal;
        bool _allow_batt_reset = false;
};

#endif
