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

#include "CCollarComms.h"
#include <string.h>
#include <stdio.h>

#define MAX_QUEUE_SIZE 4

CCollarComms::CCollarComms(uint8_t tx_pin)
{
    printf("CCollarComms(%d)\n", tx_pin);
    _pio = pio0;
    _sm  = 1;
    pio_sm_claim(_pio, _sm);
    _program_offset = pio_add_program(_pio, &collar433_program);
    collar433_program_init(_pio, _sm, _program_offset, tx_pin);
}

CCollarComms::~CCollarComms()
{
    printf("~CCollarComms()\n");
    pio_sm_set_enabled(_pio, _sm, false);
    pio_remove_program(_pio, &collar433_program, _program_offset);
    pio_sm_unclaim(_pio, _sm);
}

void CCollarComms::loop()
{
    if (!_msg_queue.empty())
    {
        if (!is_pio_fifo_queue_full())
        {
            printf("CCollarComms::loop() send queued\n");
            transmit(_msg_queue.front());
            _msg_queue.pop();
        }
    }
}

bool CCollarComms::transmit (uint16_t id, collar_channel channel, collar_mode mode, uint8_t power)
{
    collar_message msg;
    msg.id = id;
    msg.channel = channel;
    msg.mode = mode;
    msg.power = power;
    return transmit(msg);
}

bool CCollarComms::transmit (struct collar_message message)
{
    uint8_t txbuf[4]; // bytes 0+1=ID, 2=mode&channel, 3=power

    memcpy(txbuf, &message.id, 2);
    txbuf[2] = (((uint8_t)message.channel << 4) | message.mode);

    // Power levels >99 are ignored by the collar
    if (message.power > 99)
        txbuf[3] = 99;
    else
        txbuf[3] = message.power;

    // To avoid blocking, only transmit if there is space in the PIO/SMs FIFO queue.
    if (is_pio_fifo_queue_full())
    {
        if (_msg_queue.size() < MAX_QUEUE_SIZE)
        {
            // PIOs FIFO queue is full, so save in class queue for later transmission
            printf("CCollarComms::transmit: TX queued\n");
            _msg_queue.push(message);
            return true;
        }
        else
        {
            // PIO queue is full, and so is the class queue. Discard message.
            printf("CCollarComms::transmit: TX fail - not enough space in either FIFO queue\n");
            return false;
        }        
    }
    
    // The collar seems to expect to receive the same message 3 times
    tx_buffer(txbuf, sizeof(txbuf));
    tx_buffer(txbuf, sizeof(txbuf));
    tx_buffer(txbuf, sizeof(txbuf));
    _last_tx_time_us = time_us_64();
    printf("CCollarComms::transmit: TX Done\n");
    return true;
}

bool CCollarComms::is_pio_fifo_queue_full()
{
    // Transmitting a message takes 2 x 3 = 6 entries in the FIFO queue, which has a total of 8 (32bit) slots (TX & RX queues have been combined)
    return pio_sm_get_tx_fifo_level(_pio, _sm) > (8 - 6);
}

// Transmit buffer & calculate/include checksum
void CCollarComms::tx_buffer(uint8_t *buf, uint8_t buf_len)
{
    msg_part msg;
    uint8_t check=0;
  
    msg.byte[3] = 41; // Number of bits to transmit

    for (int n=0; n < buf_len; n++)
    {
        check += buf[n];
    }

    msg.byte[2] = buf[0];
    msg.byte[1] = buf[1];
    msg.byte[0] = buf[2];
    pio_sm_put_blocking(_pio, _sm, msg.data);

    msg.byte[3] = buf[3];
    msg.byte[2] = check;
    msg.byte[1] = 0;
    msg.byte[0] = 0;
    pio_sm_put_blocking(_pio, _sm, msg.data);
}

std::string CCollarComms::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case collar_mode::BEEP:
            return "Beep";
        
        case collar_mode::SHOCK:
            return "Shock";

        case collar_mode::VIBE:
            return "Vibrate";

        default:
            return "?";
    }
}
