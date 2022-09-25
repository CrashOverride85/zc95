#include "CUtil.h"
#include "globals.h"
#include "config.h"
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
