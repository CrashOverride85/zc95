#include "CUtil.h"
#include "globals.h"
#include "config.h"
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "hardware/i2c.h"

CInteruptableSection::CInteruptableSection()
{
    
}

void CInteruptableSection::start()
{
    _inital_state = gInteruptable;
    gInteruptable = true;
}

void CInteruptableSection::end()
{
    if (!_inital_state)
        gInteruptable = false;
}

const char *_mutex_held_by = NULL;

int i2c_write(const char *function_name, uint8_t addr, const uint8_t *src, size_t len, bool nostop)
{
    if (!mutex_enter_timeout_ms(&gI2cMutex, 10))
    {
        printf("i2c_write: timeout getting i2c mutex for [%s]. Mutex probably held by [%s]\n",
               function_name, _mutex_held_by ? _mutex_held_by : "UNKNOWN");
        return -1;
    }
    _mutex_held_by = function_name;

    int bytes_written = i2c_write_timeout_us(I2C_PORT, addr, src, len, nostop, 10000);
    _mutex_held_by = NULL;
    mutex_exit(&gI2cMutex);

    return bytes_written;
}

int i2c_read(const char *function_name, uint8_t addr, uint8_t *dst, size_t len, bool nostop)
{
    if (!mutex_enter_timeout_ms(&gI2cMutex, 10))
    {
        printf("i2c_read: timeout getting i2c mutex for [%s]. Mutex probably held by [%s]\n",
               function_name, _mutex_held_by ? _mutex_held_by : "UNKNOWN");
        return -1;
    }
    _mutex_held_by = function_name;

    int bytes_read = i2c_read_timeout_us(I2C_PORT, addr, dst, len, nostop, 10000);
    _mutex_held_by = NULL;
    mutex_exit(&gI2cMutex);   
    return bytes_read;
}

static void i2c_pin_deinit(uint8_t pin)
{
    gpio_disable_pulls(I2C_SDA);
    gpio_init(I2C_SDA);
    gpio_set_dir(I2C_SDA, GPIO_IN);
}

static void i2c_pin_set_state(uint8_t pin, bool high)
{
    if (high)
    {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
    else
    {
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
    }
}

// Try to reset the I2C bus. Important parts stolen/adapted from:
//   https://github.com/earlephilhower/arduino-pico/blob/6af1abc02583d7fd116091be84effe4235ac6ca7/libraries/Wire/src/Wire.cpp#L386
void i2c_reset(const char *function_name)
{
    if (!mutex_enter_timeout_ms(&gI2cMutex, 10))
    {
        printf("i2c_reset: timeout getting i2c mutex for [%s]. Mutex probably held by [%s]\n",
               function_name, _mutex_held_by ? _mutex_held_by : "UNKNOWN");
        return ;
    }
    _mutex_held_by = function_name;

    // deinit
    i2c_deinit(I2C_PORT);
    i2c_pin_deinit(I2C_SDA);
    i2c_pin_deinit(I2C_SCL);

    // Attempt bus recovery if SDA is held LOW by another device
    // See RP2040 datasheet "Bus clear feature" (not implemented in HW)
    uint8_t delay = 5; // 5us LOW/HIGH -> 10us period -> 100kHz freq
    gpio_init(I2C_SDA);
    gpio_init(I2C_SCL);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    if (!gpio_get(I2C_SDA)) // if SDA is low
    {
        printf("Attempting I2C bus recovery\n");
        uint8_t sclPulseCount = 0;
        while (sclPulseCount < 9 && !gpio_get(I2C_SDA)) 
        {
            sclPulseCount++;
            i2c_pin_set_state(I2C_SCL, 0);
            sleep_us(delay);
            i2c_pin_set_state(I2C_SCL, 1);
            sleep_us(delay);
        }

        if (gpio_get(I2C_SDA)) 
        {
            // Bus recovered : send a STOP
            i2c_pin_set_state(I2C_SDA, 0);
            sleep_us(delay);
            i2c_pin_set_state(I2C_SDA, 1);
            printf("I2C bus recovered\n");
        }
    }

    // reinit
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    _mutex_held_by = NULL;
    mutex_exit(&gI2cMutex);
}
