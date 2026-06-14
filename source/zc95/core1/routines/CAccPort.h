#ifndef _CACCPORT_H
#define _CACCPORT_H

#include "../../common/zc95_config.h"
#include "../PortExpanders/EExtInputPort.h"
#include <hardware/uart.h>
#include <pico/util/queue.h>
#include <string>

class CAccPort
{
    public:
        CAccPort();
        ~CAccPort();
        
        void io_reset();
        void io_set_port_state(enum ExtInputPort output, ExtInputPortState state);

        void serial_start();
        void serial_stop();
        void serial_reset(); // reset serial to defaults (115200/8/1/n)
        void serial_set_baud(uint32_t baudrate);
        void serial_set_format (uint8_t stop_bits, uart_parity_t parity);

        bool serial_data_available();
        char serial_get_character();
        bool serial_line_available();
        std::string serial_get_line();
        void serial_write(char character);
        void serial_write_line(std::string line);
        void serial_set_line_mode(bool enabled);

        void serial_loop();

    private:
        struct uart_inst *_uart = ACC_PORT_UART;
        int _uart_irq;
        bool _serial_started = false;

        uint32_t _baud_rate = PICO_DEFAULT_UART_BAUD_RATE;
        uint8_t _stop_bits = 1;
        uart_parity_t _parity = uart_parity_t::UART_PARITY_NONE;

        queue_t _rx_queue;
        bool _serial_mode_line = false;
        bool _serial_line_available = false;

        uint8_t _line_buffer[SERIAL_ACC_RX_QUEUE_SIZE+1];
        uint16_t _line_buffer_position;

        void serial_loop_line_mode();
        void serial_rx_irq();
        static void s_serial_rx_irq();

        static CAccPort* _s_instance;
};

#endif
