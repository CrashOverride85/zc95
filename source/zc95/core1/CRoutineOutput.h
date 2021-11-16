#ifndef _CROUTINEOUTPUT_H
#define _CROUTINEOUTPUT_H

#include <inttypes.h>
#include "routines/CRoutineMaker.h"
#include "routines/CRoutine.h"
#include "output/collar/CCollarComms.h"


class CRoutineOutput
{
    public:
        virtual void set_front_panel_power(uint8_t channel, uint16_t power) = 0;
        virtual uint16_t get_output_power(uint8_t channel) = 0;
        virtual uint16_t get_front_pannel_power(uint8_t channel) = 0;

        virtual void activate_routine(uint8_t routine_id) = 0;
        virtual void stop_routine() = 0;

        virtual void menu_min_max_change(uint8_t menu_id, int16_t new_value) = 0;
        virtual void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id) = 0;
        virtual void trigger(trigger_socket socket, trigger_part part, bool active) = 0;
        virtual void soft_button_pressed(soft_button button, bool pressed) = 0;
        virtual void loop() {};

        virtual void collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power) = 0;
        virtual void reinit_channels();
};

#endif
