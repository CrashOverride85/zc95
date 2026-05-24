#ifndef _CEXTINPUTPORTEXP_H
#define _CEXTINPUTPORTEXP_H

#include <stdio.h>

#include "IPortExpander.h"
#include "../CLedControl.h"
#include "../ZcTypes.h"
#include "EExtInputPort.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutine.h"

class CExtInputPortExp
{
    public:
        enum class Trigger
        {
            Trigger1,
            Trigger2
        };

        CExtInputPortExp(CLedControl *led, CRoutineOutput **routine_output, IPortExpander* port_exp);
        void process(bool force_update);
        bool input_state(enum ExtInputPort input);

        void clear_input();
        
        void interrupt();
        void reset_acc_port();
        void set_acc_io_port_state(enum ExtInputPort output, ExtInputPortState state);

    
    private:
        enum mkII_port_exp_reg_t
        {
            INPUT_PORT,
            OUTPUT_PORT,
            POLARITY_INVERSION,
            CONFIGURATION
        };
        
        void update_led_for_trigger_port(Trigger trigger);
        bool has_input_state_changed(enum ExtInputPort input, bool *new_state);
        void update_trigger_leds();
        void update_active_routine();
        void update_active_routine_trigger(enum ExtInputPort input, trigger_socket socket, trigger_part part);

        uint8_t _last_read;
        uint8_t _input_states_at_last_check;

        IPortExpander* _port_exp;
        int8_t _old_state;
        CLedControl *_led;
        CRoutineOutput **_routine_output;
        volatile bool _interrupt;
        uint64_t _input_last_change_time_us[8];
        uint64_t _debounce_recheck_time_us;
        zc95_version_t _hardware_version;
};

#endif
