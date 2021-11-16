// TODO: look for ACKs after sending stuff, look for connection failure + reconnect
//       + store key in eeprom

#include "C312bComms.h"
#include "pico/stdlib.h"
#include <string.h>



C312bComms::C312bComms(uart_inst_t *uart) 
{
    _uart = uart;
    uart_init(_uart, 19200);
    uart_set_hw_flow(_uart, false, false);
    uart_set_format(_uart, 8, 1, UART_PARITY_NONE);
    
    _comms_state = comms_state::NOT_CONNECTED;
    _last_tx_us = 0;
}

void C312bComms::connect()
{
    if (_comms_state == comms_state::NOT_CONNECTED || _comms_state == comms_state::FAILED)
    {
        set_comms_state(comms_state::HELLO_1);
    }
}

void C312bComms::loop()
{
    // Timeout: If the connection has been stuck at something other than connected/not-connected/failed 
    // for more than 500ms, give up and set to failed
    if ((_comms_state != comms_state::CONNECTED) && 
        (_comms_state != comms_state::NOT_CONNECTED) && 
        (_comms_state != comms_state::FAILED) &&
        (time_us_64() - _state_change_us > 500000))
    {
        set_comms_state(comms_state::FAILED);
    }

    switch(_comms_state)
    {
        case comms_state::HELLO_1:
            hello1();
            break;

        case comms_state::HELLO_2:
            hello2();
            break;

        case comms_state::HELLO_3:
            hello3();
            break;

        case comms_state::FAILED:
            if (time_us_64() - _state_change_us > 1000000) // 1 second
                set_comms_state(comms_state::HELLO_1);
            break;

        case comms_state::CONNECTED:
            // TODO: not reading anything from the box yet, so just discard anything it sends us for now
            clear_rx_buffer();
            send_tx_buffered();
            break;

        case comms_state::NOT_CONNECTED:
            break;
    }
}

bool C312bComms::send(uint16_t addr, uint8_t value)
{
    if (_comms_state != comms_state::CONNECTED)
        return false;
    
    return _tx_buffer.add(addr, value);
}

void C312bComms::send_tx_buffered()
{
    if (time_us_64() - _last_tx_us < 30000) // send data at most every 30ms
        return;

    // TODO: instead of crude timeout, should be looking for an ACK from the box before sending more data

    struct C312bTxBuffer::tx_command cmd;
    if (_tx_buffer.get(cmd))
    {
        uint8_t msg[4];

        msg[0] = 0x4d;
        msg[1] = (cmd.addr >> 8);
        msg[2] = (cmd.addr & 255);
        msg[3] = cmd.value;
        cp(msg, 4);
    }
}

bool C312bComms::is_connected()
{
    return (_comms_state == comms_state::CONNECTED);
}

void C312bComms::clear_rx_buffer()
{
    while (uart_is_readable(_uart))
    {
        uint8_t c;
        uart_read_blocking(_uart, &c, 1);
    }
}

void C312bComms::cp(uint8_t msg[], uint8_t n)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < n; i++) 
    {
        uint8_t c = msg[i];
        sum += c; // overflow expected and ok
        c ^= _mod;

        // The 32byte FIFO tx buffer means that this hopefully won't block, as we never send that much data
        uart_write_blocking(_uart, &c, 1);
        _last_tx_us = time_us_64();
    }

    if (n > 1) 
    {
        uint8_t c = sum ^ _mod;
        uart_write_blocking(_uart, &c, 1);
        _last_tx_us = time_us_64();
    }
}

void C312bComms::set_comms_state(comms_state state)
{
    _comms_state = state;

    switch (state)
    {
        case comms_state::HELLO_1:
            _hello1_tx_count = 0;
            _hello1_rx_count = 0;
            break;

        case comms_state::HELLO_2:
            _hello2_sent = false;
            break;

        case comms_state::HELLO_3:
            memset(_rx_buf, 0, sizeof(_rx_buf));
            _rx_buf_pos = 0;
            _hello3_sent = false;
            break;
        
        case comms_state::CONNECTED:
            _tx_buffer.wipe(); // just in case there's something stuck in there
           /* _command_tx_buf_position = 0;
            set_byte(ETMEM_panellock, 0x01);
            set_byte(ETMEM_knoba, 0x80); */
            send(ETMEM_panellock, 0x01);
            send(ETMEM_knoba, 0x0);
            send(ETMEM_knobb, 0x0);

            break;

        case comms_state::NOT_CONNECTED:
        case comms_state::FAILED:
            break;
    }
    
    _state_change_us = time_us_64();
}

void C312bComms::hello1()
/* send 0x00, wait for 0x07 in response. Returns true if passed this step, false if not */
{
    if (_hello1_tx_count == 0)
    {
        clear_rx_buffer();
    }

    if (time_us_64() - _last_tx_us > 30000) // 30ms
    {
        //if (_debug) _debugserial->println("tx hello");
        if (_hello1_tx_count < 10)
        {
            uint8_t send[] = {0x00};
            _mod = 0;
            cp(send, 1);
            _hello1_tx_count++;
        }
        else
        {
            set_comms_state(comms_state::FAILED);
        }
    }
    else
    {
        // If we've received 0x07 three times in a row, go to state HELLO_2
        while (uart_is_readable(_uart))
        {
            uint8_t c;
            uart_read_blocking(_uart, &c, 1);
            if (c == 0x07)
                _hello1_rx_count++;
            else 
                _hello1_rx_count = 0;
        }

        if (_hello1_rx_count > 3)
        {
            set_comms_state(comms_state::HELLO_2);
        }
    }

    return;
}

void C312bComms::hello2()
{
    if (time_us_64() - _last_tx_us < 50000)
        return;

    if (!_hello2_sent)
    {
        clear_rx_buffer();
                
        uint8_t send[] = {0x2f, 0x00};
        _mod = 0;
        cp(send, 2);
        _hello2_sent = true;
    }

    if (time_us_64() - _last_tx_us < 50000)
        return;

    uint8_t rx[MAXRXBYTES] = {0};
    uint8_t n=0;
    while (uart_is_readable(_uart) && (n < MAXRXBYTES))
    {
        uart_read_blocking(_uart, &rx[n++], 1);
    }

    int sum = rx[0] + rx[1];
    if (sum > 256) sum -= 256;
    if (n < 3 || rx[0] != 0x21 || sum != rx[2]) 
    {
        set_comms_state(comms_state::FAILED);
    }
    else
    {
        _mod = rx[1] ^ 0x55;
        set_comms_state(comms_state::HELLO_3);
    }
    
    return;
}

void C312bComms::hello3()
{
    if (!_hello3_sent)
    {
        uint8_t msg[3];
        msg[0] = 0x3c;
        msg[1] = (ETMEM_knoba >> 8);
        msg[2] = (ETMEM_knoba & 255);
        cp(msg, 3);
        _hello3_sent = true;
        return;
    }

    if (time_us_64() - _last_tx_us < 30000) // 30ms
        return;

    while (uart_is_readable(_uart) && (_rx_buf_pos < MAXRXBYTES))
    {
        uart_read_blocking(_uart, &_rx_buf[_rx_buf_pos++], 1);
    }

    if (_rx_buf_pos >= 3)
    {
        if (_rx_buf[0] != 0x22)  // first is 0x22
        {
            set_comms_state(comms_state::FAILED);
            return;
        } 

        uint8_t sum = _rx_buf[0] + _rx_buf[1]; // with valid checksum, allow overflow
        if (sum != _rx_buf[2])
        {
            set_comms_state(comms_state::FAILED);
            return;
        } 

        set_comms_state(comms_state::CONNECTED);
    }
}
