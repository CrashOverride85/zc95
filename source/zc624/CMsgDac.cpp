#include "pico/multicore.h"

#include "CMsgDac.h"
#include "messages.h"
#include <stdio.h>

CMsgDac::CMsgDac()
{
    printf("CMsgDac::CMsgDac()\n");
}

void CMsgDac::set_channel_value(dac_channel channel, uint16_t value)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_SET_DAC_POWER;
    msg.msg8[1] = (uint8_t)channel;
    msg.msg8[2] = value & 0xFF;
    msg.msg8[3] = (value >> 8) & 0xFF;

    if (multicore_fifo_wready())
    {
        multicore_fifo_push_blocking(msg.msg32);
    }
    else
    {
        printf("FIFO FULL\n");
    }
}
