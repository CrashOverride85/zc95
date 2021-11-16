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

#include "CTimingTest.h"
#include "pico/stdlib.h"
#include "stdio.h"

CTimingTest::CTimingTest()
{
    _start_time = time_us_64();
}

void CTimingTest::print_time()
{   uint64_t end_time = time_us_64();
    printf("Time: %" PRId64 "us\n", end_time - _start_time);
    _start_time = time_us_64();
}

void CTimingTest::print_time(std::string message)
{
    uint64_t end_time = time_us_64();
    printf("Time: %" PRId64 "us (%s)\n", end_time - _start_time, message.c_str());
    _start_time = time_us_64();
}
