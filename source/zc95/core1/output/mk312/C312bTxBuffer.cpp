#include "C312bTxBuffer.h"

C312bTxBuffer::C312bTxBuffer()
{
    _read_pos = 0;
    _write_pos = 0;
    _buf_len = 0;
}

bool C312bTxBuffer::add(uint16_t addr, uint8_t value)
{
    if (_buf_len >= TX_312B_BUF_SIZE)
        return false;

    _command_tx_buf[_write_pos].addr = addr;
    _command_tx_buf[_write_pos].value = value;

    _buf_len++;
    if (++_write_pos >= TX_312B_BUF_SIZE)
        _write_pos = 0;

    return true;
}

bool C312bTxBuffer::add(struct tx_command cmd)
{
    return add(cmd.addr, cmd.value);
}

void C312bTxBuffer::wipe()
{
    _read_pos = 0;
    _write_pos = 0;
    _buf_len = 0;
}

bool C312bTxBuffer::get(struct tx_command &cmd)
{
    if (_buf_len == 0)
        return false;

    cmd.addr = _command_tx_buf[_read_pos].addr;
    cmd.value = _command_tx_buf[_read_pos].value;

    _buf_len--;
    if (++_read_pos >= TX_312B_BUF_SIZE)
        _read_pos = 0;

    return true;
}
