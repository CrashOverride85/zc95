#ifndef _CMAINBOARDPORTEXP_H
#define _CMAINBOARDPORTEXP_H

#include <stdio.h>
#include "CGetButtonState.h"
#include "ECButtons.h"

class CMainBoardPortExp : public CGetButtonState
{
    public:
        CMainBoardPortExp(uint8_t address);
        void process(bool always_update);
        bool button_state(enum Button button);
        bool has_button_state_changed(enum Button button, bool *new_state);
        void interrupt();
        void clear_input();
        void set_lcd_backlight(bool on);
        void audio_input_enable(bool enable);
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);
    
    private:
        int set_pin_state(uint8_t pin, bool state);
        uint8_t _last_read;
        uint8_t _button_states_at_last_check;
        uint8_t _address;
        int8_t _old_state;
        uint64_t _last_state_change[MAX_BUTTON_IDX];
        volatile bool _interrupt;

        uint8_t _data_out = 0xFF;
};

#endif
