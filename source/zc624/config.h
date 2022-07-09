#ifndef _CCONFIG_H
#define _CCONFIG_H

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
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

// For connection to on board DAC (so we are i2c master)
#define I2C_PORT_PER i2c1
#define I2C_SDA_PER 2
#define I2C_SCL_PER 3

#define DAC_ADDRESS 0x60

// For connection to main board (so we are i2c slave)
#define I2C_PORT_SLAVE i2c0
#define I2C_SDA_SLAVE 4
#define I2C_SCL_SLAVE 5
#define ZC624_ADDR  0x10 // our address


#define DEVICE_TYPE   624
#define VERSION_MAJOR   0
#define VERSION_MINOR   2




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
