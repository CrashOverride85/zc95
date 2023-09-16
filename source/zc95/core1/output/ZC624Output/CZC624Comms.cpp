#include "../../../config.h"
#include "../../../globals.h"
#include "../../../CUtil.h"
#include "../../../CLedControl.h"
#include "../../Core1Messages.h"
#include "CZC624Comms.h"

#include "pico/multicore.h"
#include "hardware/spi.h"
#include <stdio.h>

#define SPI_BAUD_RATE 2000000 // 2mhz 

/*
 * Communicate with ZC624 output board
 */

CZC624Comms::CZC624Comms(spi_inst_t *spi, i2c_inst_t *i2c)
{
    printf("CZC624Comms()\n");
    _spi = spi;
    _i2c = i2c;

    if (_spi)
    {
        gpio_set_function(PIN_OUTPUT_BOARD_SPI_RX , GPIO_FUNC_SPI);
        gpio_set_function(PIN_OUTPUT_BOARD_SPI_SCK, GPIO_FUNC_SPI);
        gpio_set_function(PIN_OUTPUT_BOARD_SPI_TX , GPIO_FUNC_SPI);
        gpio_set_function(PIN_OUTPUT_BOARD_SPI_CSN, GPIO_FUNC_SPI);

        spi_init(_spi, SPI_BAUD_RATE);
        spi_set_format(_spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
}

CZC624Comms::~CZC624Comms()
{
    printf("~CZC624Comms()\n");
}

bool CZC624Comms::loop(uint8_t channel_id)
{
    if (spi_is_writable(_spi) && (time_us_64() - _last_msg_us > 1000))
    {
        message msg = {0};
        msg.command = (uint8_t)CZC624Comms::spi_command_t::NoOp;
        send_message(msg);
    }

    return (_led_state & (1 << channel_id));
}

void CZC624Comms::send_message(message msg)
{
    uint8_t recv[sizeof(msg)];
    if (_spi)
    {
        uint8_t *send_ptr = (uint8_t*)&msg;

        spi_write_read_blocking(_spi, send_ptr, (uint8_t*)&recv, sizeof(msg));
        _last_msg_us = time_us_64();
        
        // Whenever we send a message, we get the desired LED states for each channel in return
        _led_state = recv[0];
    }
}

bool CZC624Comms::get_i2c_register(i2c_reg_t reg, uint8_t *value)
{
    uint8_t buf[1] = {0};
    bool retval = get_i2c_register_range(reg, buf, 1);
    *value = buf[0];
    return retval;
}

bool CZC624Comms::get_i2c_register_range(i2c_reg_t reg, uint8_t *buffer, uint8_t size)
{
    uint8_t buf[1];
    buf[0] = (uint8_t)reg;

    int count = i2c_write(__func__, ZC624_ADDR, buf, 1, true);
    if (count < 0)
    {
        printf("get_i2c_register for addr=%d, reg=%d failed (write)\n", ZC624_ADDR, (uint8_t)reg);
        return false;
    }

    count = i2c_read(__func__, ZC624_ADDR, buffer, size, true);
    if (count != size)
    {
        printf("get_i2c_register for addr=%d, reg=%d failed (read; size = %d, read count = %d)\n", ZC624_ADDR, (uint8_t)reg, size, count);
        return false;
    }

    return true;
}

bool CZC624Comms::write_i2c_register(i2c_reg_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)reg;
    buf[1] = value;

    int count = i2c_write(__func__, ZC624_ADDR, buf, sizeof(buf), false);
    if (count != sizeof(buf))
    {
        printf("write_i2c_register for addr=%d, reg=%d, value=%d, bytes_written=%d (of %d) failed (write address)\n",
               ZC624_ADDR, (uint8_t)reg, value, count, sizeof(buf));
        return false;
    }

    return true;
}

bool CZC624Comms::check_zc624()
{
    uint8_t status = 0;
    if (!get_i2c_register(CZC624Comms::i2c_reg_t::OverallStatus, &status))
    {
        printf("Failed to read OverallStatus register from ZC624\n");
        return false;
    }

    // Wait for upto 2 seconds for ZC624 to become ready
    uint8_t count = 0;
    do
    {
        if (!get_i2c_register(CZC624Comms::i2c_reg_t::OverallStatus, &status))
        {
            printf("Failed to read OverallStatus register from ZC624\n");
            return false;
        }
        count++;

        if (status != CZC624Comms::status::Startup)
            break;

        sleep_ms(100);

    } while (count < 20);

    if (status != CZC624Comms::status::Ready)
    {
        printf("ZC624 is not ready (status = %d)\n", status);
        return false;
    }

    return true;
}

std::string CZC624Comms::get_version()
{
    std::string version_str = "ERROR";
    uint8_t ver_str_len = ((uint8_t)i2c_reg_t::VerStrEnd - (uint8_t)i2c_reg_t::VerStrStart);
    char* buffer = (char*)calloc(ver_str_len+1, 1);
    bool retval = get_i2c_register_range(i2c_reg_t::VerStrStart, (uint8_t*)buffer, ver_str_len);
    if (retval)
    {
        version_str = buffer;
    }

    free(buffer);
    return version_str;
}

bool CZC624Comms::get_major_minor_version(uint8_t *major, uint8_t *minor)
{
    bool retval = true;
    retval &= get_i2c_register(CZC624Comms::i2c_reg_t::VersionMajor, major);
    retval &= get_i2c_register(CZC624Comms::i2c_reg_t::VersionMinor, minor);

    return retval;
}

