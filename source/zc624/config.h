#ifndef _CCONFIG_H
#define _CCONFIG_H

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

/*

    SPIx_SCK = SCK
    SPIx_TX = MOSI
    SPIx_RX = MISO
*/

#define SPI_PORT spi0
#define PIN_SPI_MOSI 16 // RX
#define PIN_SPI_CS   17
#define PIN_SPI_SCK  18
#define PIN_SPI_MISO 19 // TX
#define SPI_BAUD_RATE 500000


// I2C defines
#define I2C_PORT_PER i2c1
#define I2C_SDA_PER 2
#define I2C_SCL_PER 3

#define DAC_ADDRESS 0x60


#define PIN_9V_ENABLE   14

#define PIN_CHAN1_GATE_A 6
#define PIN_CHAN1_GATE_B 7

#define PIN_CHAN2_GATE_A 8
#define PIN_CHAN2_GATE_B 9

#define PIN_CHAN3_GATE_A 10
#define PIN_CHAN3_GATE_B 11

#define PIN_CHAN4_GATE_A 12
#define PIN_CHAN4_GATE_B 13

#endif
