#ifndef _CCORE1MESSAGES_H
#define _CCORE1MESSAGES_H

#include "pico/mutex.h"

// Messages from core0 to core1
#define MESSAGE_ROUTINE_LOAD                  1
#define MESSAGE_ROUTINE_MIN_MAX_CHANGE        2
#define MESSAGE_ROUTINE_MULTI_CHOICE_CHANGE   3
#define MESSAGE_ROUTINE_TRIGGER               4
#define MESSAGE_SET_FRONT_PANNEL_POWER        5
#define MESSAGE_TRIGGER_COLLAR                6
#define MESSAGE_ROUTINE_STOP                  7
#define MESSAGE_ROUTINE_SOFT_BUTTON_PUSHED    8
#define MESSAGE_REINIT_CHANNELS               9

// messages from core1 to core0
#define MESSAGE_SET_POWER                   100
#define MESSAGE_SET_MAXIMUM_POWER           101

#define MESSAGE_SET_LED_CHAN0               110
#define MESSAGE_SET_LED_CHAN1               111
#define MESSAGE_SET_LED_CHAN2               112
#define MESSAGE_SET_LED_CHAN3               113

#define MESSAGE_SET_ACC_IO_PORT_RESET       120
#define MESSAGE_SET_ACC_IO_PORT1_STATE      121
#define MESSAGE_SET_ACC_IO_PORT2_STATE      122
#define MESSAGE_SET_ACC_IO_PORT3_STATE      123


union __attribute__((packed)) message
{
    uint32_t msg32;
    uint8_t msg8[4];
};


void messages_init();

#endif
