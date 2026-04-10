#include "eeprom.h"
#include <stdbool.h>
#include <stdio.h>
#include "i2c.h"
#include "hardware/i2c.h"

uint8_t get_eeprom_flag()
{
    uint16_t address = EEPROM_BOOTLOADER_SETTING_ADDR;
    uint8_t data[1];
    data[0] = address & 0xFF;


    int bytes_written = i2c_write(__func__, (uint8_t)(EEPROM_ADDR | ((address >> 8) & 0x07)), data, sizeof(data), false);
    if (bytes_written != sizeof(data))
    {
        printf("EEPROM i2c read (write addr) failed! i2c bytes_written = %d\n", bytes_written);
        return 0;
    }
    int ret = i2c_read(__func__, (uint8_t)(EEPROM_ADDR | ((address >> 8) & 0x07)), data, sizeof(data), false);

    if (ret != sizeof(data))
    {
        printf("EEPROM read failed! i2c read returned %d\n", ret);
        return 0;
    }

    if (data[0] > EEPROM_BOOTLOADER_SETTING_RESTORE_PREV)
    {
        return EEPROM_BOOTLOADER_SETTING_NORMAL;
    }

    return data[0];
}

bool set_eeprom_flag(uint8_t val)
{
    uint16_t address = EEPROM_BOOTLOADER_SETTING_ADDR;
    uint8_t data[2];
    data[0] = address & 0xFF;
    data[1] = val;

    uint8_t bytes_written = i2c_write_timeout_us(I2C_PORT, (uint8_t)(EEPROM_ADDR | ((address >> 8) & 0x07)), data, sizeof(data), false, 2000);
    
    // The eeprom chip will ignore all commands until the write is complete. 
    // wait for write to complete
    int ret;
    int retry_count=0;
    do
    {
        ret = i2c_read(__func__, (uint8_t)(EEPROM_ADDR | ((address >> 8) & 0x07)), data, sizeof(data), false);
    } while ((ret < 0) && (retry_count++ < 100));
    if (ret < 0)
    {
        printf("EEPROM write didn't complete?!\n");
        return false;
    }

    return (bytes_written == sizeof(data));
}

