#include <stdio.h>
#include "hardware/timer.h"

enum xmodem_state
{
    XMODEM_IDLE,
    XMODEM_RECV_STARTED,
    XMODEM_RECV_BLOCK,
    XMODEM_RECV_COMPLETE
}; 

enum xmodem_debug_level
{
    XMODEM_DEBUG_DEBUG,
    XMODEM_DEBUG_INFO,
    XMODEM_DEBUG_ERROR
};

#define XMODEM_BUFFER_SIZE (128+5)

#define XMODEM_SOH 0x01
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06 
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_C   0x43

 // callback used to send serial data
typedef void (xmodem_tx_cb)(uint8_t character, void* user);

// callback used to pass received data back to caller. Expect length to always be 128.
typedef bool (xmodem_got_data_cb)(const uint8_t* data, size_t length, void* user);

// callback used to output debug data
typedef void (xmodem_debug_cb)(const uint8_t* str, enum xmodem_debug_level level, void* user);


struct xmodem_ctx
{
    enum xmodem_state state;
    uint8_t buffer[XMODEM_BUFFER_SIZE];
    uint16_t buf_pos;
    uint8_t current_blk;
    uint32_t total_blocks_received;
    xmodem_tx_cb* send;
    xmodem_got_data_cb* process_data;
    xmodem_debug_cb* debug_cb;
    uint64_t last_activity_time_us;
    uint8_t error_count; // Consecutive error count. Reset to 0 when sending an ACK
    bool use_crc;

    void* user;
};

void xmodem_init(struct xmodem_ctx* ctx, void* user, xmodem_tx_cb* tx_func, xmodem_got_data_cb* process_data_func, xmodem_debug_cb* debug_cb);
void xmodem_receive(struct xmodem_ctx* ctx);
void xmodem_loop(struct xmodem_ctx* ctx);
void xmodem_serial_rx(struct xmodem_ctx* ctx, uint8_t character);
