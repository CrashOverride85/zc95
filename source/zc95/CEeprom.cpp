/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "CEeprom.h"
#include <stdio.h>

/*
 * Read/Write from EEPROM U1. Assumes/tested with AT24C04C, but probably works for anything similar
 */

CEeprom::CEeprom(i2c_inst_t *i2c, uint8_t i2c_address)
{
    _i2c_address = i2c_address;
    _i2c = i2c;
}

uint8_t CEeprom::read(uint16_t address)
{
    uint8_t data[1];
    data[0] = address & 0xFF;
    int bytes_written = i2c_write_timeout_us(_i2c, (uint8_t)(_i2c_address | ((address >> 8) & 0x07)), data, sizeof(data), false, 2000);
    if (bytes_written != sizeof(data))
    {
        printf("CEeprom::read failed! i2c bytes_written = %d\n", bytes_written);
        return 0;
    }

    int ret = i2c_read_timeout_us(_i2c, (uint8_t)(_i2c_address | ((address >> 8) & 0x07)), data, sizeof(data), false, 2000);
    if (ret != sizeof(data))
    {
        printf("CEeprom::read failed! i2c read returned %d\n", ret);
        return 0;
    }

    return data[0];
}

bool CEeprom::write(uint16_t address, uint8_t value, bool block_until_complete)
{
    uint8_t data[2];
    data[0] = address & 0xFF;
    data[1] = value;
    uint8_t bytes_written = i2c_write_timeout_us(_i2c, (uint8_t)(_i2c_address | ((address >> 8) & 0x07)), data, sizeof(data), false, 2000);
    
    // The eeprom chip will ignore all commands until the write is complete. So if block_until_complete is set, don't
    // return until it starts responding again.
    if (block_until_complete)
    {
        // wait for write to complete
        int ret;
        int retry_count=0;
        do
        {
            ret = i2c_read_timeout_us(_i2c, (uint8_t)(_i2c_address | ((address >> 8) & 0x07)), data, sizeof(data), false, 200);
        } while ((ret < 0) && (retry_count++ < 100));
        if (ret < 0)
        {
            printf("CEeprom::write write didn't complete?!");
            return false;
        }
    }
    
    return (bytes_written == sizeof(data));
}

