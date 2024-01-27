#include "CRotEnc.h"
#include "CGetButtonState.h"
#include "../CSavedSettings.h"
#include "../config.h"

#ifndef _CFRONTPANEL_H
#define _CFRONTPANEL_H

class CFrontPanel : CGetButtonState
{
    public:
        enum interrupt_t
        {
            INT1 = 1,
            INT2 = 2
        };

        virtual void process(bool always_update) = 0;
        virtual uint16_t get_channel_power_level(uint8_t channel) = 0;
        virtual int8_t get_adjust_control_change() = 0;
        virtual bool has_button_state_changed(enum Button button, bool *new_state) = 0;
        virtual void interrupt (interrupt_t i) = 0;
};

#endif
