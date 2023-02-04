#ifndef _CDISPLAYMESSAGE_H
#define _CDISPLAYMESSAGE_H

#include "CMenu.h"
#include "CDisplay.h"
#include "CDisplayMessage.h"

class CDisplayMessage : public CMenu
{
    public:
        CDisplayMessage(CDisplay* display, CGetButtonState *buttons, std::string message);
        ~CDisplayMessage();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        std::string word_wrap(std::string text, unsigned per_line);

        std::string _display_string;
        struct display_area _disp_area;
        
        CDisplay* _display;
        CGetButtonState *_buttons;
};

#endif
