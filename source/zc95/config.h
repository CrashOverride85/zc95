
#define PULSE_QUEUE_LENGTH 12  // queue size for pules sent from core0 to core1. Only used for audio pattern
                               // (maybe serial input in the future), where processing happens on core0

#define PATTERN_TEXT_OUTPUT_QUEUE_LENGTH 4 // Max number of queued text/debug messages from Core1 (so far just lua) to Core0

#define ZC624_ADDR                  0x10
#define EXT_INPUT_PORT_EXP_ADDR     0x21 // 3x I/O lines on front panel accessory port (p0-02), 4x for trigger inputs (p4-p7), 1x N/C (p3)
#define CONTROLS_PORT_EXP_ADDR      0x22 // 4x front panel buttons (p0-p03), 3x I/O lines on expansion header J17 (p4-p6), 1x LCD backlight (p7)
#define FP_ANALOG_PORT_EXP_2_ADDR   0x26 // Port expander (U2) on the front panel
#define AUDIO_DIGIPOT_ADDR          0x2C // Digital potentiometer used to set gain on audio board
#define ADC_ADDR                    0x48 // ADC on front panel, used for power control dials
#define EEPROM_ADDR                 0x50 


#define I2C_PORT i2c0  // main i2c bus for port expanders + eeprom

// Set expected version for zc624 output module
#define ZC624_REQUIRED_MAJOR_VERION 2
#define ZC624_MIN_MINOR_VERION      0

// Versions for the GetVersion/VersionDetails message, but that's not used by anything yet
#define WEBSOCKET_API_VERION_MAJOR  1   // Increment on breaking change
#define WEBSOCKET_API_VERION_MINOR  0   // Increment non-breaking change

// The maximum length a web socket message can be in bytes
#define MAX_WS_MESSAGE_SIZE 300

// How many Lua instructions can be run on each Lua call. Protects against infinite loops locking up box.
#define LUA_MAX_INSTRUCTIONS 10000

// Other pins
#define PIN_LED           10 // ws2812 LED chain
#define PIN_CONTROLS_INT   7 // front panel controls port expander interrupt pin
#define PIN_EXT_INPUT_INT 21 // external inputs port expander interrupt pin
#define PIN_433TX          3 // 433Mhz transmitter pin

#define PIN_FP_INT1       11 // front panel interrupt 1 (U1) - 4x channel rot encoders
#define PIN_FP_INT2        6 // front panel interrupt 2 (U2) - 5x rot encoder buttons & 1x adjust rot encoder (+ 1x unused line)

// Output board SPI
#define ZC624_SPI_PORT            spi1
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

// For the time being, allowing more than one web socket connection is likely to cause all sorts of issues, 
// e.g. fighting for control over running routine, possibility to change a lua script whilst it's running, etc.
#define MAX_WEBSOCKET_CONNECTIONS 1 

// Note that just changing these two values would likely break everything
#define MAX_POWER_LEVEL 1000
#define MAX_CHANNELS 4

// SPI Defines for display
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
#define AUX_PORT_UART uart0

#define PIN_ACC_UART_TX 8
#define PIN_ACC_UART_RX 9
#define ACC_PORT_UART uart1

#define SERIAL_TX_QUEUE_SIZE 2000 // only used when in remote access/serial mode
