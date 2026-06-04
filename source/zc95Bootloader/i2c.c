#include "i2c.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

void i2c_start()
{
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

int i2c_write(const char *function_name, uint8_t addr, const uint8_t *src, size_t len, bool nostop)
{
    // PICO_ERROR_TIMEOUT = -2
    int bytes_written = i2c_write_timeout_us(I2C_PORT, addr, src, len, nostop, 100000);
    if (bytes_written != len)
    {
        printf("i2c_write error: bytes_written = %d vs %d expected\n", bytes_written, len);
    }
    else
    {
        // printf("i2c: wrote %d bytes\n", bytes_written);
    }
    return bytes_written;
}

int i2c_read(const char *function_name, uint8_t addr, uint8_t *dst, size_t len, bool nostop)
{
    int bytes_read = i2c_read_timeout_us(I2C_PORT, addr, dst, len, nostop, 100000);  
    return bytes_read;
}


bool get_i2c_register_range(uint8_t reg, uint8_t* buffer, uint8_t size)
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
