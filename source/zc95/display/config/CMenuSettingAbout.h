#ifndef _CMENUSETTINGABOUT_H
#define _CMENUSETTINGABOUT_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "../HwCheck/CHwCheck.h"
#include "../../Hal/IHal.h"

class CMenuSettingAbout : public CMenu
{
    public:
        CMenuSettingAbout(CDisplay* display, IHal *hal, CHwCheck *hwCheck);
        ~CMenuSettingAbout();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        void put_text_line(std::string text, int16_t x, int16_t y, uint8_t line, hagl_color_t colour);
        CDisplay* _display;
        IHal *_hal;
        CHwCheck *_hwCheck;
};

#endif
