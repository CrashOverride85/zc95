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
 */

#include "pico/multicore.h"
#include "hardware/irq.h"
#include <stdio.h>
#include "CAccPort.h"
#include "../Core1Messages.h"
#include <string.h>

CAccPort* CAccPort::_s_instance = NULL;

CAccPort::CAccPort()
{
    printf("CAccPort()\n");
    _uart_irq = _uart == uart0 ? UART0_IRQ : UART1_IRQ;
    memset(_line_buffer, 0, sizeof(_line_buffer));
    _line_buffer_position = 0;
    _serial_mode_line = false;
    _s_instance = this;
}

CAccPort::~CAccPort()
{
    printf("~CAccPort()\n");
    _s_instance = NULL;

    if (_serial_started)
        serial_stop();
}

void CAccPort::io_reset()
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_SET_ACC_IO_PORT_RESET;
    msg.msg8[1] = 0;
    msg.msg8[2] = 0;
    msg.msg8[3] = 0;

    multicore_fifo_push_blocking(msg.msg32);
}

void CAccPort::io_set_port_state(enum ExtInputPort output, ExtInputPortState state)
{
    uint8_t message_type = 0;
    switch (output)
    {
        case ExtInputPort::ACC_IO_1: message_type = MESSAGE_SET_ACC_IO_PORT1_STATE; break;
        case ExtInputPort::ACC_IO_2: message_type = MESSAGE_SET_ACC_IO_PORT2_STATE; break;
        case ExtInputPort::ACC_IO_3: message_type = MESSAGE_SET_ACC_IO_PORT3_STATE; break;
        default:
            return;
    }

    message msg = {0};
    msg.msg8[0] = message_type;
    msg.msg8[1] = (uint8_t)state;
    msg.msg8[2] = 0;
    msg.msg8[3] = 0;

    if (multicore_fifo_wready())
        multicore_fifo_push_blocking(msg.msg32);
    else
        printf("CAccPort::io_set_port_state: queue full\n");
}

void CAccPort::serial_start()
{
    if (_serial_started)
        return;

    queue_init(&_rx_queue, sizeof(char), SERIAL_ACC_RX_QUEUE_SIZE);
    uart_init(_uart, _baud_rate);
    uart_set_format(_uart, 8, _stop_bits, _parity);
    irq_set_exclusive_handler(_uart_irq, &CAccPort::s_serial_rx_irq);
    irq_set_enabled(_uart_irq, true);
    uart_set_irqs_enabled(_uart, true, false);
    
    _serial_started = true;
}

void CAccPort::serial_stop()
{
    if (!_serial_started)
        return;

    uart_set_irqs_enabled(_uart, false, false);
    irq_set_enabled(_uart_irq, false);
    irq_remove_handler(_uart_irq, &CAccPort::s_serial_rx_irq);
    uart_deinit(_uart);
    queue_free(&_rx_queue);

    _serial_started = false;
}

void CAccPort::serial_reset()
{
    if (!_serial_started)
        return;

     uart_set_hw_flow(_uart, false, false);
     serial_set_baud(PICO_DEFAULT_UART_BAUD_RATE);
     serial_set_format(1, uart_parity_t::UART_PARITY_NONE);
     _serial_mode_line = false;
     
    while (!queue_is_empty(&_rx_queue))
        queue_remove_blocking(&_rx_queue, NULL);
}

void CAccPort::serial_set_baud(uint32_t baudrate)
{
    _baud_rate = baudrate;

    if (_serial_started)
        uart_set_baudrate(_uart, baudrate);
}

void CAccPort::serial_set_format(uint8_t stop_bits, uart_parity_t parity)
{
    _stop_bits = stop_bits;
    _parity = parity;

    if (_serial_started)
        uart_set_format(_uart, 8, _stop_bits, _parity);
}

bool CAccPort::serial_data_available()
{
    return !queue_is_empty(&_rx_queue);
}

char CAccPort::serial_get_character()
{
    char ch = '\0';
    queue_try_remove(&_rx_queue, &ch);
    return ch;
}

bool CAccPort::serial_line_available()
{
    return _serial_line_available;
}

std::string CAccPort::serial_get_line()
{
    if (_serial_line_available)
    {
        _line_buffer[SERIAL_ACC_RX_QUEUE_SIZE] = '\0';
        std::string line = std::string((char*)_line_buffer);

        _serial_line_available = false;
        memset(_line_buffer, 0, sizeof(_line_buffer));
        _line_buffer_position = 0;

        serial_loop();

        return line;
    }
    else
        return "";
}

void CAccPort::serial_write(char character)
{
    if (_serial_started)
        uart_putc_raw(_uart, character);
}

void CAccPort::serial_write_line(std::string line)
{
    if (!_serial_started)
        return;

    for(std::string::iterator it = line.begin(); it != line.end(); ++it)
    {
        uart_putc_raw(_uart, *it);
    }
    uart_putc_raw(_uart, '\r');
    uart_putc_raw(_uart, '\n');
}

void CAccPort::serial_set_line_mode(bool enabled)
{
    _serial_mode_line = enabled;
}

void CAccPort::serial_loop()
{
    if (_serial_mode_line)
        serial_loop_line_mode();
}

void CAccPort::serial_loop_line_mode()
{
    if (_serial_line_available)
        return;

    char ch = '\0';

    while (queue_try_remove(&_rx_queue, &ch))
    {
        if (ch == '\r')
        {
            _line_buffer[_line_buffer_position] = '\0';
            _serial_line_available = true;
            return;
        } 
        else if (ch == '\n')
            continue;

        if (_line_buffer_position >= SERIAL_ACC_RX_QUEUE_SIZE - 1)
        {
            // overrun
            _line_buffer_position = 0;
            memset(_line_buffer, 0, sizeof(_line_buffer));
            return;
        }

        _line_buffer[_line_buffer_position++] = ch;
    }
}

void CAccPort::s_serial_rx_irq()
{
    if (_s_instance)
        _s_instance->serial_rx_irq();
}

void CAccPort::serial_rx_irq()
{
    while (uart_is_readable(_uart))
    {
        uint8_t ch = uart_getc(_uart);
        queue_try_add(&_rx_queue, &ch);
    }
}
