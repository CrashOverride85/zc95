/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
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

/* 
 * Handle serial communication on Aux/Serial port. Messages (in and out) are prefixed with
 * STX and end with ETX. Other than that, the format (JSON) is the same as the websocket
 * interface.
 * 
 * Processing of incoming messages is handled by CMessageProcessor.
 */

#include <ArduinoJson.h>
#include <list>
#include "CSerialConnection.h"
#include "../git_version.h"

CSerialConnection *CSerialConnection::_s_instance;

CSerialConnection::CSerialConnection(uart_inst_t *uart, CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines)
{
    printf("CSerialConnection::CSerialConnection()\n");
    _s_instance = this;
    _uart = uart;

    _recv_buffer_position = 0;
    _tx_buffer_position = 0;
    memset(_recv_buffer, 0, sizeof(_recv_buffer));
    memset(_tx_buffer, 0, sizeof(_tx_buffer));
    _tx_in_progress = false;

    uart_init(_uart, PICO_DEFAULT_UART_BAUD_RATE);
    _uart_irq = _uart == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(_uart_irq, CSerialConnection::s_irq_handler);
    irq_set_enabled(_uart_irq, true);
    uart_set_irq_enables(_uart, true, true);

    _messageProcessor = new CMessageProcessor(analogue_capture, routine_output, routines, std::bind(&CSerialConnection::send, this, std::placeholders::_1));
}

CSerialConnection::~CSerialConnection()
{
    printf("~CSerialConnection()\n");
    
    uart_set_irq_enables(_uart, false, false);
    irq_set_enabled(_uart_irq, false);
    irq_remove_handler(_uart_irq, CSerialConnection::s_irq_handler);
    _s_instance = NULL;

    if (_messageProcessor != NULL)
    {
        delete _messageProcessor;
        _messageProcessor = NULL;
    }
}

void CSerialConnection::on_uart_irq()
{
    // Serial transmit
    while (uart_is_writable(_uart) && _tx_buffer[_tx_buffer_position])
    {
        uart_putc_raw(_uart, _tx_buffer[_tx_buffer_position++]);
    }
    
    if (_tx_buffer[_tx_buffer_position] == NULL)
    {
        _tx_in_progress = false;
    }

    // Serial receive
    while (uart_is_readable(_uart)) 
    {
        uint8_t ch = uart_getc(_uart);

        switch (_state)
        {
            case state_t::IDLE:
                if (ch == STX)
                    _state = state_t::RECV;
                break;
            
            case state_t::RECV:
                if ((_recv_buffer_position > MAX_WS_MESSAGE_SIZE) || (_recv_buffer_position > sizeof(_recv_buffer)+2)) 
                {
                    printf("CSerialConnection::on_uart_rx(): max buffer length reached, discarding message\n");
                    _recv_buffer_position = 0;
                    memset(_recv_buffer, 0, sizeof(_recv_buffer));
                    _state = state_t::IDLE;
                }
                else
                {
                    if (ch == ETX)
                    {
                        printf("Got serial message\n");
                        _state = state_t::GOT_MESSAGE;
                    }
                    else
                        _recv_buffer[_recv_buffer_position++] = ch;
                }
                break;
        }
    }

    // Clear TX interrupt so it doesn't continually fire after first tx and cause a lockup
    // see https://stackoverflow.com/a/76393711
    uart_get_hw(_uart)->icr = UART_UARTICR_TXIC_BITS;
}

void CSerialConnection::s_irq_handler()
{
    if (_s_instance)
        _s_instance->on_uart_irq();
}

void CSerialConnection::loop()
{
    if (_state == state_t::GOT_MESSAGE)
    {
        _messageProcessor->message(_recv_buffer, _recv_buffer_position);
        _recv_buffer_position = 0;
        memset(_recv_buffer, 0, sizeof(_recv_buffer_position));
        _state = state_t::IDLE;
    }

    if (_messageProcessor)
        _messageProcessor->loop();
}

void CSerialConnection::send(std::string message)
{
    if (_tx_in_progress)
    {
        printf("CSerialConnection::send() error - transmit already in progress\n");
        return;
    }

    uint16_t max_message_len = sizeof(_tx_buffer)+3;
    if (message.length() > max_message_len)
    {
        printf("CSerialConnection::send() error - message too long (%d vs limit of %d)\n", message.length(), max_message_len);
        return;
    }
    
    printf("msg > %s\n", message.c_str());

    _tx_buffer_position = 0;
    memset(_tx_buffer, 0, sizeof(_tx_buffer_position));
    sprintf((char*)_tx_buffer, "%c%s%c", STX, message.c_str(), ETX);

    // Send as much as possible now (filling tx FIFO buffer)
    while (uart_is_writable(_uart) && _tx_buffer[_tx_buffer_position])
    {
        uart_putc_raw(_uart, _tx_buffer[_tx_buffer_position++]);
    }

    // If there's any left, will get sent via interrupt 
    if (_tx_buffer[_tx_buffer_position])
    {
        _tx_in_progress = true;
    }
}
