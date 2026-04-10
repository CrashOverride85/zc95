#ifndef _BUFFERED_SERIAL_
#define _BUFFERED_SERIAL_

#include <stdio.h>
#include <hardware/uart.h>
#include <pico/util/queue.h>

struct buffered_serial_ctx
{
    uart_inst_t* uart;
    queue_t rx_queue;
};

void bs_init(struct buffered_serial_ctx* ctx, uart_inst_t *uart);
void bs_deinit(struct buffered_serial_ctx* ctx);
void bs_write(struct buffered_serial_ctx* ctx, const void *buf, size_t nbyte);
void bs_printf(struct buffered_serial_ctx* ctx, const char *format, ...);
size_t bs_read(struct buffered_serial_ctx* ctx, void *buf, size_t nbyte);
void bs_clear_rx_buffer(struct buffered_serial_ctx* ctx);

#endif
