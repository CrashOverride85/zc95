#ifndef _CEXTINPUTPORTEXP_H
#define _CEXTINPUTPORTEXP_H

#include <stdio.h>

#include "CLedControl.h"
#include "EExtInputPort.h"
#include "core1/CRoutineOutput.h"
#include "core1/routines/CRoutine.h"

class CExtInputPortExp
{
    public:
        enum class Trigger
        {
            Trigger1,
            Trigger2
        };

        CExtInputPortExp(uint8_t address, CLedControl *led, CRoutineOutput *routine_output);
        void process(bool force_update);
        bool input_state(enum ExtInputPort input);

        void clear_input();
        bool get_trigger_state(Trigger trigger);
        void interrupt();
        void reset_acc_port();
        void set_acc_io_port_state(enum ExtInputPort output, bool high);

    
    private:
        bool has_input_state_changed(enum ExtInputPort input, bool *new_state);
        bool has_input_been_triggered(enum ExtInputPort input);
        void update_trigger_leds();
        void update_active_routine();
        void update_active_routine_trigger(enum ExtInputPort input, trigger_socket socket, trigger_part part);

        uint8_t _last_read;
        uint8_t _input_states_at_last_check;

        uint8_t _address;
        int8_t _old_state;
        CLedControl *_led;
        CRoutineOutput *_routine_output;
        volatile bool _interrupt;
        uint64_t _input_last_change_time_us[8];
        uint8_t _output_mask = 0xFF;
};

#endif
