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

#include "CChannelConfig.h"

CChannelConfig::CChannelConfig(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control)
{
    printf("CChannelConfig()\n");
    _saved_settings = saved_settings;
    _power_level_control = power_level_control;
}

CChannelConfig::~CChannelConfig()
{
    printf("~CChannelConfig()\n");
}

void CChannelConfig::loop()
{
    _collar_comms.loop();
}

CCollarComms* CChannelConfig::get_collar_comms()
{
    return &_collar_comms;
}

void CChannelConfig::configure_channels_from_saved_config(COutputChannel** active_channels)
{
    for (int channel_id=0; channel_id < MAX_CHANNELS; channel_id++)
    {
        // Clear all channels to start with
        if (active_channels[channel_id] != NULL)
        {
            delete active_channels[channel_id];
            active_channels[channel_id] = NULL;
        }

        CSavedSettings::channel_selection channel_details = _saved_settings->get_channel(channel_id);

        switch (channel_details.type)
        {
            case  CChannel_types::channel_type::CHANNEL_COLLAR:
                // FIXME: read collar ID from eeprom:
                active_channels[channel_id] = new CCollarChannel(_saved_settings, &_collar_comms, _power_level_control, channel_id); 
                break;

            case  CChannel_types::channel_type::CHANNEL_INTERNAL:
                active_channels[channel_id] = new CZC624ChannelFull(_saved_settings, &_zc614_comms, _power_level_control, channel_id);
                break;

            default:
                printf("CChannelConfig::configure_channels_from_saved_config(): Error - unexpected channel type encountered\n");
                break;
        }
    }
}

void CChannelConfig::shutdown_zc624()
{
    printf("shutting down zc624 output module\n");
    CZC624Comms::message message = {0};
    message.command = (uint8_t)CZC624Comms::spi_command_t::PowerDown;
    _zc614_comms.send_message(message);
}
