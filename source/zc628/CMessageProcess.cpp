#include "CMessageProcess.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

CMessageProcess::CMessageProcess(COutput *output)
{
    printf("CMessageProcess()\n");
    _output = output;
}

CMessageProcess::~CMessageProcess()
{
    printf("~CMessageProcess()\n");
}

void CMessageProcess::init()
{
    // SPI initialisation
    spi_init(SPI_PORT, SPI_BAUD_RATE);
    

   // spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1,SPI_MSB_FIRST);
	spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    spi_set_slave(SPI_PORT, true);

    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_CS,   GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
}

void CMessageProcess::loop()
{
    message msg;

    spi_read_blocking(SPI_PORT, (uint8_t)0x00, (uint8_t*)&msg, 4);
//printf("command = %d, arg 0=%d, 1=%d, 2=%d\n", msg.command, msg.arg0, msg.arg1, msg.arg2);
    switch ((command)msg.command)
    {
        case command::Pulse:
            pulse(msg);
            break;

        case command::SetPower:
            set_power(msg);
            break;

        default:
            printf("ERR: command = %d, arg 0=%d, 1=%d, 2=%d\n", msg.command, msg.arg0, msg.arg1, msg.arg2);
            break;
    }

}

// Pulse args:
// 0 = channel (0-3)
// 1 = +ve pulse len, us (0-255)
// 2 = -ve pulse len, us (0-255)
void CMessageProcess::pulse(message msg)
{
    _output->pulse(msg.arg0, msg.arg1, msg.arg2);
}

// SetPower args:
// 0 = channel (0-3)
// 1+2 = power level (0-1000)
void CMessageProcess::set_power(message msg)
{
    uint16_t power = msg.arg1 << 8 | msg.arg2;

    // printf("PWR C=%d -> %d\n", msg.arg0, power);
    _output->set_power(msg.arg0, power);
}
