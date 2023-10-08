#include "CMessageProcess.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

CMessageProcess::CMessageProcess(COutput *output, CI2cSlave *i2c_slave)
{
    printf("CMessageProcess()\n");
    _output = output;
    _i2c_slave = i2c_slave;
}

CMessageProcess::~CMessageProcess()
{
    printf("~CMessageProcess()\n");
}

void CMessageProcess::init()
{
    // SPI initialisation
    spi_init(SPI_PORT, SPI_BAUD_RATE);

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

    spi_read_blocking(SPI_PORT, _output->get_channel_led_state(), (uint8_t*)&msg, 4);
// printf("command = %d, arg 0=%d, 1=%d, 2=%d\n", msg.command, msg.arg0, msg.arg1, msg.arg2);
    switch ((command)msg.command)
    {
        case command::Pulse:
            pulse(msg);
            break;

        case command::SetPower:
            set_power(msg);
            break;

        case command::PowerDown:
            power_down(msg);
            break;

        case command::SetFreq:
            set_freq(msg);
            break;

        case command::SetPulseWidth:
            set_pulse_width(msg);
            break;

        case command::SwitchOn:
            on(msg);
            break;

        case command::SwitchOff:
            off(msg);
            break;

        case command::NoOp:
            // Sent so we can send LED states 
            break;

        case command::SetTestVal:
            // Used so zc95 can test that SPI comms are working (at least to some extent) on startup. It'll use i2c to read this value back.
            _i2c_slave->set_value((uint8_t)CI2cSlave::reg::TestVal, msg.arg0);
            break;

        default:
            printf("ERR: command = %d, arg 0=%d, 1=%d, 2=%d\n", msg.command, msg.arg0, msg.arg1, msg.arg2);
            break;
    }
}

// Pulse - generate a single pulse. Shouldn't be used with SwitchOn/SwitchOff
// Args:
// 0 = channel (0-3)
// 1 = +ve pulse len, us (0-255)
// 2 = -ve pulse len, us (0-255)
void CMessageProcess::pulse(message msg)
{
    _output->pulse(msg.arg0, msg.arg1, msg.arg2);
}

// SetPower - set output power
// Args:
// 0 = channel (0-3)
// 1+2 = power level (0-1000)
void CMessageProcess::set_power(message msg)
{
    uint16_t power = msg.arg1 << 8 | msg.arg2;
    _output->set_power(msg.arg0, power);
}

// PowerDown - Power down the zc624 output module: Stop any output and switch off 9v supply
// Args: 
// None
void CMessageProcess::power_down(message msg)
{
    _output->power_down();
}


// SetFreq - set output frequency generated if SwitchOn used
// Args:
// 0 = channel (0-3)
// 1+2 = frequency (Hz)
void CMessageProcess::set_freq(message msg)
{
    uint16_t freq = msg.arg1 << 8 | msg.arg2;
    
    if (freq != 0)
        _output->set_freq(msg.arg0, freq);
}

// SetPulseWidth - set pulse width generated if SwitchOn used
// Args:
// 0 = channel (0-3)
// 1 = pos pulse width (us)
// 2 = neg pulse width (us)
void CMessageProcess::set_pulse_width(message msg)
{
    _output->set_pulse_width(msg.arg0, msg.arg1, msg.arg2);
}

// SwitchOn - switch output on, using previously set frequence / pulse width. Shouldn't be used with pulse
// Args:
// 0 = channel (0-3)
void CMessageProcess::on(message msg)
{
    _output->on(msg.arg0);
}

// SwitchOff - switch output off. Shouldn't be used with pulse
// 0 = channel (0-3)
void CMessageProcess::off(message msg)
{
    _output->off(msg.arg0);
}
