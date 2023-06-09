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
 * If an EOT is received, this is treated similar to a websocket disconnect - any running
 * routine is stopped, and state cleared.
 * 
 * Note that this class sets up an interrupt handler for the UART passed in, and keeps a static
 * reference to itself. I.e. creating more than one instance of this class won't work.
 * 
 * Processing of incoming messages is handled by CMessageProcessor.
 */

#include <ArduinoJson.h>
#include <list>
#include "CSerialConnection.h"
#include "../git_version.h"

extern CSerialConnection *g_SerialConnection;
CSerialConnection *CSerialConnection::_s_instance = NULL;

CSerialConnection::CSerialConnection(uart_inst_t *uart, CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines)
{
    printf("CSerialConnection::CSerialConnection()\n");
    _s_instance = this;
    _uart = uart;
    _analogue_capture = analogue_capture;
    _routine_output = routine_output;
    _routines = routines;

    _recv_buffer_position = 0;
    memset(_recv_buffer, 0, sizeof(_recv_buffer));

    _reset_connection = false;
    queue_init(&_tx_queue, sizeof(char), SERIAL_TX_QUEUE_SIZE);

    uart_init(_uart, PICO_DEFAULT_UART_BAUD_RATE);
    _uart_irq = _uart == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(_uart_irq, CSerialConnection::s_irq_handler);
    irq_set_enabled(_uart_irq, true);
    uart_set_irq_enables(_uart, true, true);

    _messageProcessor = new CMessageProcessor(_analogue_capture, _routine_output, _routines, std::bind(&CSerialConnection::send, this, std::placeholders::_1));
    g_SerialConnection = this;
}

CSerialConnection::~CSerialConnection()
{
    printf("~CSerialConnection()\n");
    g_SerialConnection = NULL;
    
    uart_set_irq_enables(_uart, false, false);
    irq_set_enabled(_uart_irq, false);
    irq_remove_handler(_uart_irq, CSerialConnection::s_irq_handler);
    _s_instance = NULL;

    if (_messageProcessor != NULL)
    {
        delete _messageProcessor;
        _messageProcessor = NULL;
    }

    queue_free(&_tx_queue);
}

void CSerialConnection::on_uart_irq()
{
    // Serial transmit
    while (uart_is_writable(_uart) && !queue_is_empty(&_tx_queue))
    {
        char ch;
        bool got_entry = queue_try_remove(&_tx_queue, &ch);
        if (got_entry)
            uart_putc_raw(_uart, ch);
    }

    // Serial receive
    while (uart_is_readable(_uart)) 
    {
        uint8_t ch = uart_getc(_uart);

        if (ch == EOT)
        {
            printf("EOT received, reset connection\n");
            _reset_connection = true;
        }

        // If an EOT has been received, ignore (discard) any more incoming data until the connection has been reset
        if (!_reset_connection)
        {
            switch (_state)
            {
                case state_t::IDLE:
                    if (ch == STX)
                        _state = state_t::RECV;
                    break;
                
                case state_t::RECV:
                    if ((_recv_buffer_position > MAX_WS_MESSAGE_SIZE) || (_recv_buffer_position > sizeof(_recv_buffer)-2)) 
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

                case state_t::GOT_MESSAGE:
                case state_t::RESET:
                    break;
            }
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
        
        if (_reset_connection)
            _state = state_t::RESET;
        else
            _state = state_t::IDLE;
    }

    if (_messageProcessor)
        _messageProcessor->loop();

    if (_reset_connection && _state != state_t::GOT_MESSAGE)
    {
        printf("CSerialConnection::loop(): start reset\n");
        // Disable serial interrupts, wait for tx buffer to empty then clear rx buffer
        uart_set_irq_enables(_uart, false, false);
        while (!queue_is_empty(&_tx_queue));

        while (uart_is_readable(_uart)) 
            uart_getc(_uart);

        // Reset message processor - if a routine is running, this will stop it
        if (_messageProcessor)
            delete _messageProcessor;

        _messageProcessor = new CMessageProcessor(_analogue_capture, _routine_output, _routines, std::bind(&CSerialConnection::send, this, std::placeholders::_1));

        _state = state_t::IDLE;
        _reset_connection = false;
        uart_set_irq_enables(_uart, true, true);

        printf("CSerialConnection::loop(): reset done\n");
    }
}

void CSerialConnection::send(std::string message)
{
    const char *msg_ptr = message.c_str();
    printf("send > %s\n", message.c_str());

    uint space_in_queue = SERIAL_TX_QUEUE_SIZE - queue_get_level(&_tx_queue);
    if (message.length() > space_in_queue)
    {
        printf("CSerialConnection::send() error - not enough space in queue (%d message size vs space %d)\n", message.length(), space_in_queue);
        return;
    }

    queue_add_blocking(&_tx_queue, &STX);
    while (*msg_ptr)
        queue_add_blocking(&_tx_queue, msg_ptr++);
    queue_add_blocking(&_tx_queue, &ETX);

    irq_set_pending(_uart_irq);
}
