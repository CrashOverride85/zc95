/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "pico/multicore.h"
#include <stdio.h>
#include "CAccPort.h"
#include "../Core1Messages.h"

CAccPort::CAccPort()
{
    printf("CAccPort()\n");
}

CAccPort::~CAccPort()
{
    printf("~CAccPort()\n");
}

void CAccPort::reset()
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_SET_ACC_IO_PORT_RESET;
    msg.msg8[1] = 0;
    msg.msg8[2] = 0;
    msg.msg8[3] = 0;

    multicore_fifo_push_blocking(msg.msg32);
}

void CAccPort::set_io_port_state(enum ExtInputPort output, bool high)
{
    uint8_t messsge_type = 0;
    switch (output)
    {
        case ExtInputPort::ACC_IO_1: messsge_type = MESSAGE_SET_ACC_IO_PORT1_STATE; break;
        case ExtInputPort::ACC_IO_2: messsge_type = MESSAGE_SET_ACC_IO_PORT2_STATE; break;
        case ExtInputPort::ACC_IO_3: messsge_type = MESSAGE_SET_ACC_IO_PORT3_STATE; break;
        default:
            return;
    }

    message msg = {0};
    msg.msg8[0] = messsge_type;
    msg.msg8[1] = (uint8_t)high;
    msg.msg8[2] = 0;
    msg.msg8[3] = 0;

    if (multicore_fifo_wready())
        multicore_fifo_push_blocking(msg.msg32);
    else
        printf("CAccPort::set_io_port_state: queue full\n");
}
