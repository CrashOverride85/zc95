#ifndef _CMAINBOARDPORTEXP_H
#define _CMAINBOARDPORTEXP_H

#include <stdio.h>
#include <cstdint>
#include "IPortExpander.h"
#include "../CGetButtonState.h"
#include "../ECButtons.h"
#include "../ZcTypes.h"

class CMainBoardPortExp : public CGetButtonState
{
    public:
        CMainBoardPortExp(zc95_version_t hardware_version, IPortExpander* port_exp);
        void process(bool always_update);
        bool button_state(enum Button button);
        bool has_button_state_changed(enum Button button, bool *new_state);
        void interrupt();
        void clear_input();
        void set_lcd_backlight(bool on);
        void audio_input_enable(bool enable);
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

        bool get_tp4056_charge_status();
        bool get_tp4056_standby_status();
    
    private:
        uint8_t _last_read;
        uint8_t _button_states_at_last_check;
        uint8_t _address;
        int8_t _old_state;
        uint64_t _last_state_change[MAX_BUTTON_IDX];
        volatile bool _interrupt;
        zc95_version_t _hardware_version;
        IPortExpander* _port_exp;

        const int MicPreampEnablePin = 4;
        const int MicPowerDisablePin = 5;

        // MK1
        const int AudioInputEnablePin = 6;
        const int BackLightPin = 7;

        // MK2
        const int StandbyPin = 0;
        const int ChargePin = 1;
};

#endif
