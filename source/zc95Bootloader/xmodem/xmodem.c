/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
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
 * 
 * 
 * XMODEM/CRC implementation for uploading firmware to the zc95 
 * over serial.
 * Only supports XMODEM/CRC with 128 byte blocks. Won't automatically 
 * fallback to checksum instead of CRC, although there is support in 
 * this library to support checksum instead of CRC.
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "xmodem.h"
#include <string.h>
#include <stdbool.h>


static void process_buffer(struct xmodem_ctx* ctx);
static void debug_printf(struct xmodem_ctx* ctx, enum xmodem_debug_level level, const char *fmt, ...);
static uint16_t calculate_crc(uint8_t *ptr, size_t count);

#define XMODEM_TIMEOUT_SECONDS 5

void xmodem_init(struct xmodem_ctx* ctx, void* user, xmodem_tx_cb* tx_func, xmodem_got_data_cb* process_data_func, xmodem_debug_cb* debug_cb)
{
    ctx->state = XMODEM_IDLE;
    memset(ctx->buffer, 0, XMODEM_BUFFER_SIZE);
    ctx->buf_pos = 0;
    ctx->current_blk = 1;
    ctx->send = tx_func;
    ctx->process_data = process_data_func;
    ctx->user = user;
    ctx->debug_cb = debug_cb;
    ctx->last_activity_time_us = 0;
    ctx->error_count = 0;
    ctx->total_blocks_received = 0;
    ctx->use_crc = true;
}

void xmodem_receive(struct xmodem_ctx* ctx)
{
    debug_printf(ctx, XMODEM_DEBUG_INFO, "Waiting for file\n");
    ctx->state = XMODEM_RECV_STARTED;

    // With CRC, transfers are started by sending a C 
    if (ctx->use_crc)
        ctx->send(XMODEM_C, ctx->user);
    else
        // If using checksum instead, send NAK (this does not indicate an error)
        ctx->send(XMODEM_NAK, ctx->user);

    ctx->last_activity_time_us = time_us_64();
}

void xmodem_loop(struct xmodem_ctx* ctx)
{
    if (ctx->state == XMODEM_RECV_STARTED || ctx->state == XMODEM_RECV_BLOCK)
    {
        if (time_us_64() - ctx->last_activity_time_us > (1000 * 1000 * XMODEM_TIMEOUT_SECONDS))
        {
            if (ctx->error_count >= 10)
            {
                debug_printf(ctx, XMODEM_DEBUG_ERROR, "Too many errors, giving up\n");
                ctx->state = XMODEM_IDLE;
                ctx->send(XMODEM_CAN, ctx->user);
                ctx->error_count = 0;
            }
            else
            {
                ctx->error_count++;
                if (ctx->use_crc && ctx->total_blocks_received == 0)
                {
                    debug_printf(ctx, XMODEM_DEBUG_ERROR, "Timeout, sending C (error count = %d)\n", ctx->error_count);
                    ctx->send(XMODEM_C, ctx->user);
                }
                else
                {
                    debug_printf(ctx, XMODEM_DEBUG_ERROR, "Timeout, sending NAK (error count = %d)\n", ctx->error_count);
                    ctx->send(XMODEM_NAK, ctx->user);
                }
                ctx->last_activity_time_us = time_us_64();
            }
        }
    }
}

void xmodem_serial_rx(struct xmodem_ctx* ctx, uint8_t character)
{
    switch(ctx->state)
    {
        case XMODEM_RECV_STARTED:
        {
            if (character == XMODEM_SOH)
            {
                ctx->state = XMODEM_RECV_BLOCK;
                ctx->buffer[0] = character;
                ctx->buf_pos = 1;
            }
            else if (character == XMODEM_EOT)
            {
                ctx->state = XMODEM_RECV_COMPLETE;
                memset(ctx->buffer, 0, XMODEM_BUFFER_SIZE);
                ctx->buf_pos = 0;
                ctx->process_data(NULL, 0, ctx->user); // signal done receiving
                ctx->send(XMODEM_ACK, ctx->user);
                ctx->last_activity_time_us = time_us_64();
                debug_printf(ctx, XMODEM_DEBUG_DEBUG, "ACK EOT\n");
                debug_printf(ctx, XMODEM_DEBUG_INFO, "File received!\n");
            }
            else if (character == XMODEM_CAN)
            {
                // CAN will only work to cancel if we're waiting for a block to start
                debug_printf(ctx, XMODEM_DEBUG_ERROR, "Received CAN (CTRL-X), aborting.\n");
                ctx->send(XMODEM_CAN, ctx->user);
                ctx->process_data(NULL, 0, ctx->user); // signal done receiving
                ctx->state = XMODEM_IDLE;
            }
        }
        break;

        case XMODEM_RECV_BLOCK:
        {
            ctx->buffer[ctx->buf_pos++] = character;
            if
            (
                ( ctx->use_crc && ctx->buf_pos >= XMODEM_BUFFER_SIZE  ) ||
                (!ctx->use_crc && ctx->buf_pos >= XMODEM_BUFFER_SIZE-1)  // if using checksum not CRC, expect 1 byte less (checksum is 1 byte, CRC is 2)
            )
            {
                process_buffer(ctx);
                ctx->state = XMODEM_RECV_STARTED;
                memset(ctx->buffer, 0, XMODEM_BUFFER_SIZE);
                ctx->buf_pos = 0;
            }
        }
    }

    ctx->last_activity_time_us = time_us_64();
}

// Process receive buffer, which should include a full line - starting with SOH and ending with a checksum or CRC.
// If it looks valid, call process_received_data_block() and ACK the line. Otherwise, send a NAK.
static void process_buffer(struct xmodem_ctx* ctx)
{
    bool isValid = false;
    bool dup = false;

    // Validate buffer
    do
    {
        if (ctx->buffer[0] != XMODEM_SOH)
            break;
        
        uint8_t cmp_blk = ~ctx->buffer[1];
        if (ctx->buffer[2] != cmp_blk)
        {
            debug_printf(ctx, XMODEM_DEBUG_ERROR, "ERR: unexpected ~blk (got 0x%x expected 0x%x)\n", ctx->buffer[2], cmp_blk);
            break;
        }

        // If the received block id is one less than we're expecting, then it's a repeat of the previous
        // block. Just ACK it (asuming the rest is valid), but don't do anything with the data.
        if (ctx->buffer[1] == ctx->current_blk - 1)
        {
            debug_printf(ctx, XMODEM_DEBUG_ERROR, "WRN: blk 0x%x duplicated\n", ctx->buffer[1]);
            dup = true;
        } 
        else if (ctx->buffer[1] != ctx->current_blk)
        {
            debug_printf(ctx, XMODEM_DEBUG_ERROR, "ERR: unexpected blk (got 0x%x expected 0x%x)\n", ctx->buffer[1], ctx->current_blk);
            break;
        }

        if (ctx->use_crc)
        {
            uint16_t calculated_crc = calculate_crc(&ctx->buffer[3], 128);                
            uint16_t received_crc = ctx->buffer[XMODEM_BUFFER_SIZE-1];
            received_crc |= ctx->buffer[XMODEM_BUFFER_SIZE-2] << 8;

            if (received_crc != calculated_crc)
            {
                debug_printf(ctx, XMODEM_DEBUG_ERROR, "ERR: unexpected CRC (got 0x%x expected 0x%x)\n", received_crc, calculated_crc);
                break;
            }
        }
        else
        {
            uint8_t checksum = 0;
            for (size_t n=0; n < 128; n++)
                checksum += ctx->buffer[n+3];

            if (ctx->buffer[XMODEM_BUFFER_SIZE-2] != checksum)
            {
                debug_printf(ctx, XMODEM_DEBUG_ERROR, "ERR: unexpected checksum (got 0x%x expected 0x%x)\n", ctx->buffer[XMODEM_BUFFER_SIZE-2] , checksum);
                break;
            }
        }

        isValid = true;

    } while(0);


    if (isValid)
    {
        debug_printf(ctx, XMODEM_DEBUG_DEBUG, "ACK %d\n", ctx->current_blk);
        
        if (!dup)
        {
            ctx->total_blocks_received++;

            if (ctx->current_blk == 1)
            {
                if (ctx->total_blocks_received == 1)
                    debug_printf(ctx, XMODEM_DEBUG_INFO, "Receiving file\n");
                else
                    debug_printf(ctx, XMODEM_DEBUG_INFO, "Receiving file (%d k bytes)\n", (ctx->total_blocks_received * 128) / 1024);
            }

            bool result = ctx->process_data(&ctx->buffer[3], 128, ctx->user);
            if (!result)
            {
                // Supplied callback function failed. Cancel the transfer.
                debug_printf(ctx, XMODEM_DEBUG_ERROR, "Error writing file, aborting\n");
                ctx->send(XMODEM_CAN, ctx->user);
                ctx->state = XMODEM_IDLE;
                return;
            }
            ctx->current_blk++;
        }
        ctx->send(XMODEM_ACK, ctx->user);
        ctx->last_activity_time_us = time_us_64();
        ctx->error_count = 0;
    }
    else
    {
        ctx->send(XMODEM_NAK, ctx->user);
        ctx->last_activity_time_us = time_us_64();
        debug_printf(ctx, XMODEM_DEBUG_DEBUG, "NAK\n");
    }
}

// Taken from crc16_ccitt in:
// https://github.com/Thuffir/xmodem/blob/master/xmodem.c
static uint16_t calculate_crc(uint8_t* ptr, size_t count)
{
  uint16_t crc16 = 0;
  while(count != 0) 
  {
    crc16  = (uint8_t)(crc16 >> 8) | (crc16 << 8);
    crc16 ^= *ptr;
    crc16 ^= (uint8_t)(crc16 & 0xff) >> 4;
    crc16 ^= (crc16 << 8) << 4;
    crc16 ^= ((crc16 & 0xff) << 4) << 1;
    ptr++;
    count--;
  }

  return crc16;
}

static void debug_printf (struct xmodem_ctx* ctx,enum xmodem_debug_level level,  const char *fmt, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, fmt);

    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    if (ctx->debug_cb != NULL)
        ctx->debug_cb(buffer, level, ctx->user);
}
