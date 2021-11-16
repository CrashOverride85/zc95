#ifndef _CMENU_H
#define _CMENU_H

#include "../CControlsPortExp.h"
#include "../ECButtons.h"

class CMenu
{
    public:
        enum class action
        {
            NONE,   // Continue showing menu
            BACK    // menu done, return to previous level
        };

        virtual ~CMenu();

        action update();
        virtual void show() = 0;
        virtual void button_pressed(Button button) = 0;
        virtual void button_released(Button button) {};
        virtual void adjust_rotary_encoder_change(int8_t change) = 0;

    protected:
        virtual void draw() = 0;
        void set_active_menu(CMenu *new_submenu);
        bool _exit_menu = false;
        CMenu *_submenu_active = NULL;
};

#endif
