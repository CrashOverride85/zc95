#ifndef _CFPKNOBS_H
#define _CFPKNOBS_H

#include <inttypes.h>

/* Virtual class to allow switching between the two types of front pannel:
 * - CFpAnalog - 4x potentiometers, 1x rotary encoder
 * - CFpRotEnc - 5x rotary encoders. Only real "benift" is all through hole parts And, *maybe* smaller steps in power (although the 255 of the ADC should be enough)
 */

class CFpKnobs
{
    public:
        enum port_exp
        {
            U1 = 0,
            U2 = 1
        };

        virtual void process(bool always_update) = 0;
        virtual uint16_t get_channel_power_level(uint8_t channel) = 0;
        virtual int8_t get_adjust_control_change() = 0;
        virtual void interupt (port_exp exp) = 0;
};
 #endif
