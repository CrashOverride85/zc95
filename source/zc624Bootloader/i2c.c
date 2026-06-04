#include "i2c.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

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

    int bytes_written = i2c_write_timeout_us(I2C_PORT, addr, src, len, nostop, 10000);
    return bytes_written;
}

int i2c_read(const char *function_name, uint8_t addr, uint8_t *dst, size_t len, bool nostop)
{
    int bytes_read = i2c_read_timeout_us(I2C_PORT, addr, dst, len, nostop, 10000);  
    return bytes_read;
}
