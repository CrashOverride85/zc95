/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
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

#include "CHwCheckZc624.h"
#include "config.h"
#include <stdio.h>

bool CHwCheckZc624::check_zc624()
{
    uint8_t status = 0;
    if (!get_i2c_register(ZC624_ADDR, (uint8_t)CHwCheckZc624::reg::OverallStatus, &status))
    {
        printf("Failed to read OverallStatus register from ZC624\n");
        return false;
    }

    // Wait for upto 2 seconds for ZC624 to become ready
    uint8_t count = 0;
    do
    {
        if (!get_i2c_register(ZC624_ADDR, (uint8_t)CHwCheckZc624::reg::OverallStatus, &status))
        {
            printf("Failed to read OverallStatus register from ZC624\n");
            return false;
        }
        count++;

        if (status != CHwCheckZc624::status::Startup)
            break;

        sleep_ms(100);

    } while (count < 20);

    if (status != CHwCheckZc624::status::Ready)
    {
        printf("ZC624 is not ready (status = %d)\n", status);
        return false;
    }

    return true;
}

std::string CHwCheckZc624::get_version()
{
    std::string version_str = "ERROR";
    uint8_t ver_str_len = ((uint8_t)reg::VerStrEnd - (uint8_t)reg::VerStrStart);
    char* buffer = (char*)calloc(ver_str_len+1, 1);
    bool retval = get_i2c_register_range(ZC624_ADDR, (uint8_t)reg::VerStrStart, (uint8_t*)buffer, ver_str_len);
    if (retval)
    {
        version_str = buffer;
    }

    free(buffer);
    return version_str;
}

bool CHwCheckZc624::get_i2c_register(uint8_t address, uint8_t reg, uint8_t *value)
{
    uint8_t buf[1] = {0};
    bool retval = get_i2c_register_range(address, reg, buf, 1);
    *value = buf[0];
    return retval;
}

bool CHwCheckZc624::get_i2c_register_range(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t size)
{
    uint8_t buf[1];
    buf[0] = reg;

    int count = i2c_write_blocking(i2c0, address, buf, 1, true);
    if (count < 0)
    {
        printf("get_i2c_register for addr=%d, reg=%d failed (write)\n", address, reg);
        return false;
    }

    count = i2c_read_blocking(i2c0, address, buffer, size, true);
    if (count != size)
    {
        printf("get_i2c_register for addr=%d, reg=%d failed (read)\n", address, reg);
        return false;
    }

    return true;
}
