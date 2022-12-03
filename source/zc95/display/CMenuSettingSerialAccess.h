#ifndef _CMENUSETTINGSERIALACCESS_H
#define _CMENUSETTINGSERIALACCESS_H

#include "CMenu.h"
#include "CDisplay.h"
#include "../CAnalogueCapture.h"
#include "../core1/CRoutineOutput.h"

extern mutex_t g_core1_suspend_mutex;
extern struct semaphore g_core1_suspend_sem;

class CMenuSettingSerialAccess : public CMenu
{
    public:
        CMenuSettingSerialAccess(CDisplay* display, CGetButtonState *buttons, CRoutineOutput *routine_output, CAnalogueCapture *analogueCapture);
        ~CMenuSettingSerialAccess();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        void put_text_line(std::string text, int16_t x, int16_t y, uint8_t line, color_t colour);

        void do_update();
        void write_flash();

        CDisplay* _display;
        CGetButtonState *_buttons;
        CRoutineOutput *_routine_output;
        CAnalogueCapture *_analogueCapture;
};

#endif
