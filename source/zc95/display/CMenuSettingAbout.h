#ifndef _CMENUSETTINGABOUT_H
#define _CMENUSETTINGABOUT_H

#include "CMenu.h"
#include "CDisplay.h"

class CMenuSettingAbout : public CMenu
{
    public:
        CMenuSettingAbout(CDisplay* display, CGetButtonState *buttons);
        ~CMenuSettingAbout();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        CDisplay* _display;
        CGetButtonState *_buttons;
};

#endif
