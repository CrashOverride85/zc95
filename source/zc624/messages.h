#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <inttypes.h>


#define  MESSAGE_SET_DAC_POWER 1


union __attribute__((packed)) message
{
    uint32_t msg32;
    uint8_t msg8[4];
};

#endif

