#include "../../../config.h"
#include "CZC1Comms.h"

#include "hardware/spi.h"

#include <stdio.h>

#define SPI_BAUD_RATE 2000000 // 2mhz 

/*
 * Communicate with ZC624 output board
 */

CZC1Comms::CZC1Comms(spi_inst_t *spi)
{
    printf("CZC1Comms()\n");
    _spi = spi;

    gpio_set_function(PIN_OUTPUT_BOARD_SPI_RX , GPIO_FUNC_SPI);
    gpio_set_function(PIN_OUTPUT_BOARD_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_OUTPUT_BOARD_SPI_TX , GPIO_FUNC_SPI);
    gpio_set_function(PIN_OUTPUT_BOARD_SPI_CSN, GPIO_FUNC_SPI);

    spi_init(_spi, SPI_BAUD_RATE);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
}

CZC1Comms::~CZC1Comms()
{
    printf("~CZC1Comms()\n");
}

void CZC1Comms::send_message(message msg)
{
    uint8_t *ptr = (uint8_t*)&msg;
    spi_write_blocking(_spi, ptr, sizeof(msg));
}
