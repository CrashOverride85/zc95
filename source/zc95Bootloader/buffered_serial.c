#include "buffered_serial.h"
#include <stdio.h>
#include <stdarg.h>
#include <hardware/uart.h>
#include <hardware/irq.h>

static struct buffered_serial_ctx* _ctx;

static void irq_handler(struct buffered_serial_ctx* ctx)
{
    bool done_something = false;

    // Serial receive
    while (uart_is_readable(ctx->uart))
    {
        uint8_t ch = uart_getc(ctx->uart);
        if (queue_is_full(&ctx->rx_queue))
        {
            printf("!");
        }
        else
            queue_add_blocking(&ctx->rx_queue, &ch);

        done_something = true;
    }

    if (!done_something)
    {
        // Something about writing flash (presumably disabling / re-enabling interrupts) causes this interrupt to 
        // continuously fire, despite the uart not being readable. No amount of clearing interrupts here seemed
        // to fix it, but re-init'ing the uart did.
        bs_init(ctx, ctx->uart);
    }
}

static void s_irq_handler()
{
    irq_handler(_ctx);
}

void bs_init(struct buffered_serial_ctx* ctx, uart_inst_t *uart)
{
    _ctx = ctx;
    ctx->uart = uart;
    queue_init(&ctx->rx_queue, sizeof(char), 1000); 

    uart_init(ctx->uart, PICO_DEFAULT_UART_BAUD_RATE);
    uart_set_hw_flow(ctx->uart, false, false);
    uart_set_format(ctx->uart, 8, 1, UART_PARITY_NONE);

    uint uart_irq = ctx->uart == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, s_irq_handler);
    irq_set_enabled(uart_irq, true);
    uart_set_irq_enables(ctx->uart, true, false);
}

void bs_deinit(struct buffered_serial_ctx* ctx)
{
    uart_set_irq_enables(ctx->uart, false, false);
    uint uart_irq = ctx->uart == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(uart_irq, false);
    irq_remove_handler(uart_irq, s_irq_handler);

    queue_free(&ctx->rx_queue);
}

void bs_write(struct buffered_serial_ctx* ctx, const void *buf, size_t nbyte)
{
    uint8_t *out = (uint8_t *)buf;

    for(size_t n=0; n < nbyte; n++)
    {
        uart_putc_raw(ctx->uart, out[n]);
    }
}

void bs_printf(struct buffered_serial_ctx* ctx, const char *format, ...) 
{
    char buffer[256];

    va_list args;
    va_start(args, format);
    int bytes = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for(size_t n=0; n < bytes; n++)
    {
        if (buffer[n] == '\n')
        {
            uart_putc_raw(ctx->uart, '\r');
            uart_putc_raw(ctx->uart, '\n');
        }
        else
            uart_putc_raw(ctx->uart, buffer[n]);
    }
}

size_t bs_read(struct buffered_serial_ctx* ctx, void *buf, size_t nbyte)
{
    uint8_t *out = (uint8_t *)buf;
    for (size_t n = 0; n < nbyte; n++)
    {
        if (queue_is_empty(&ctx->rx_queue))
            return n;

        queue_remove_blocking(&ctx->rx_queue, &out[n]);
    }

    return nbyte;
}

void bs_clear_rx_buffer(struct buffered_serial_ctx* ctx)
{
    while (!queue_is_empty(&ctx->rx_queue))
        queue_remove_blocking(&ctx->rx_queue, NULL);
}
