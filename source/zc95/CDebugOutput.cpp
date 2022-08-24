#include "CDebugOutput.h"
#include "config.h"
#include "pico/stdlib.h"

void CDebugOutput::set_debug_destination(debug_dest_t destination)
{
    switch(destination)
    {
        case debug_dest_t::ACC:
            stdio_uart_init_full(uart1, PICO_DEFAULT_UART_BAUD_RATE, PIN_ACC_UART_TX, PIN_ACC_UART_RX);
            break;

        case debug_dest_t::AUX:
            stdio_uart_init_full(uart0, PICO_DEFAULT_UART_BAUD_RATE, PIN_AUX_UART_TX, PIN_AUX_UART_RX);
            break;

        case debug_dest_t::OFF:
        default:
            stdio_set_driver_enabled(&stdio_uart, false);
            break;
    }
}

void CDebugOutput::set_debug_destination_from_settings(CSavedSettings *saved_settings)
{
    switch(saved_settings->get_debug_dest())
    {
        case CSavedSettings::setting_debug::ACC_PORT:
            printf("Setting debug destination to Accessory port\n");
            sleep_ms(10);
            set_debug_destination(debug_dest_t::ACC);
            break;

        case CSavedSettings::setting_debug::AUX_PORT:
            printf("Setting debug destination to Aux port\n");
            sleep_ms(10);
            set_debug_destination(debug_dest_t::AUX);
            break;

        case CSavedSettings::setting_debug::OFF:
            printf("Disabling debug output\n");
            sleep_ms(10);
            set_debug_destination(debug_dest_t::OFF);
            break;
    }
}
