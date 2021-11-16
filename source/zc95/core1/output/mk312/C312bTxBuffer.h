#include <inttypes.h>

#define TX_312B_BUF_SIZE 5

class C312bTxBuffer
{
    public:
        C312bTxBuffer();

       struct tx_command
        {
            uint16_t addr;
            uint8_t value;
        };
        
        bool add(uint16_t addr, uint8_t value);
        bool add(struct tx_command cmd);
        bool get(struct tx_command &cmd);
        void wipe();

    private:
        uint8_t _read_pos;
        uint8_t _write_pos;
        uint8_t _buf_len;
        struct tx_command _command_tx_buf[TX_312B_BUF_SIZE];

};

