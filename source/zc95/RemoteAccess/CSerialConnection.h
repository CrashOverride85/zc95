#ifndef _CSERIALCONNECTION_H
#define _CSERIALCONNECTION_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include "httpd.h"
#include "CLuaLoad.h"
#include "CRoutineRun.h"
#include "CMessageProcessor.h"
#include "../config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CSerialConnection
{
    public:
        CSerialConnection(uart_inst_t *uart, CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines);
        ~CSerialConnection();
        // void callback(uint8_t *data, u16_t data_len, uint8_t mode);
        void send(std::string message);
        void loop();
        static CSerialConnection *_s_instance;

    private:
        void on_uart_irq();
        static void s_irq_handler();

        const uint8_t STX = 0x02;
        const uint8_t ETX = 0x03;
        const uint8_t EOT = 0x04;

        enum class state_t
        {
            IDLE,        // Waiting for STX
            RECV,        // STX received, getting data
            GOT_MESSAGE, // ETX received, got message ready to be processed
            RESET        // EOT received, reset connection (stopping routine from running if started)
        };

        CRoutineOutput *_routine_output; 
        std::vector<CRoutines::Routine>& _routines;

        CMessageProcessor *_messageProcessor;
        volatile state_t _state = state_t::IDLE;

        int _uart_irq;    
        struct uart_inst *_uart;
        uint8_t _recv_buffer[MAX_WS_MESSAGE_SIZE+3]; // +2 for STX/ETX, +1 for NULL
        uint16_t _recv_buffer_position;
        volatile bool _reset_connection;
        queue_t _tx_queue;
};

#endif
