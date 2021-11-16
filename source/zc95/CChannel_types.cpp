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

#include "CChannel_types.h"


std::string CChannel_types::get_channel_name(channel_type type, uint8_t index)
{
    switch (type)
    {
        case channel_type::CHANNEL_NONE:
            return "None";

        case channel_type::CHANNEL_INTERNAL:
            return "Internal CHAN" + std::to_string(index+1);

        case channel_type::CHANNEL_COLLAR:
            return "Collar " + std::to_string(index+1);

        case channel_type::CHANNEL_312:
            if (index == 0)
                return "312B Output A";
            else if (index == 1)
                return "312B Output B";
            else
                return "";

        default:
            return "";
    }
}

