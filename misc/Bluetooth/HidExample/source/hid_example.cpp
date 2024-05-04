#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "bluetooth_send.h"


uint8_t get_adc_reading()
{
    adc_select_input(0);
    uint16_t reading = adc_read();
    return reading >> 4; // convert 12-bit ADC reading to 8-bit
}

int main()
{
    stdio_init_all();
    printf("ZC95 bluetooth HID test project\n");
    cyw43_arch_init();

    // ADC init
    adc_init();
    adc_gpio_init(26);

    BluetoothSend bt = BluetoothSend();
    bt.setup();

    uint8_t prev_adc_reading = 0;
    uint8_t adc_reading = 0;

    while(1)
    {
        cyw43_arch_poll();

        adc_reading = get_adc_reading();
        if (adc_reading != prev_adc_reading)
        {
            bt.send(adc_reading);
            cyw43_arch_poll();

            prev_adc_reading = adc_reading;
        }
    }

    return 0;
}
