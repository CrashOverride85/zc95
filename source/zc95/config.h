
#define PULSE_QUEUE_LENGTH 8  // queue size for pules sent from core0 to core1. Only used for audio pattern
                           // (maybe serial input in the future), where processing happens on core0

#define ZC624_ADDR                  0x10
#define EXT_INPUT_PORT_EXP_ADDR     0x21 // 3x I/O lines on front panel accesory port (p0-02), 4x for trigger inputs (p4-p7), 1x N/C (p3)
#define CONTROLS_PORT_EXP_ADDR      0x22 // 4x front pannel buttons (p0-p03), 3x I/O lines on expansion header J17 (p4-p6), 1x LCD backlight (p7)
#define FP_ANALOG_PORT_EXP_2_ADDR   0x26 // Port expander (U2) on the front pannel
#define AUDIO_DIGIPOT_ADDR          0x2C // Digital potentiometer used to set gain on audio board
#define ADC_ADDR                    0x48 // ADC on front pannel, used for power control dials
#define EEPROM_ADDR                 0x50 


#define I2C_PORT i2c0  // main i2c bus for port expanders + eeprom

// Set expected version for zc624 output module
#define ZC624_REQUIRED_MAJOR_VERION 0
#define ZC624_MIN_MINOR_VERION      3

// Other pins
#define PIN_LED           10 // ws2812 LED chain
#define PIN_CONTROLS_INT   7 // front pannel controls port expander interrupt pin
#define PIN_EXT_INPUT_INT 21 // external inputs port expander interrupt pin
#define PIN_433TX          3 // 433Mhz transmitter pin

#define PIN_FP_INT1       11 // front panel interupt 1 (U1) - 4x channel rot encoders
#define PIN_FP_INT2        6 // front panel interupt 2 (U2) - 5x rot enoder buttons & 1x adjust rot encoer (+ 1x unused line)


// Output board SPI
#define PIN_OUTPUT_BOARD_SPI_RX   12
#define PIN_OUTPUT_BOARD_SPI_SCK  14
#define PIN_OUTPUT_BOARD_SPI_TX   15
#define PIN_OUTPUT_BOARD_SPI_CSN  13

// For routines that don't care about pulse width and/or frequency, what to use
// TODO: maybe move these two to config options and save in EEPROM
#define DEFAULT_FREQ_HZ 150
#define DEFAULT_PULSE_WIDTH 150

#define TRIGGER_INPUT_DEBOUNCE_US 10000 // 10ms
#define RAMP_UP_TIME_MAXIMUM_SECS 30    // the maximum ramp up time that can be configured in seconds (0-255)

// Note that just changing these two values would likely break everything
#define MAX_POWER_LEVEL 1000
#define MAX_CHANNELS 4


// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_SDA 8
#define I2C_SCL 9

#define PIN_AUX_UART_TX 0
#define PIN_AUX_UART_RX 1

#define PIN_ACC_UART_TX 8
#define PIN_ACC_UART_RX 9

