/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
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

CChannelConfig::CChannelConfig(CSavedSettings *saved_settings)
{
    printf("CChannelConfig()\n");
    _power_level_control = NULL;
    _saved_settings = saved_settings;
}

CChannelConfig::~CChannelConfig()
{
    printf("~CChannelConfig()\n");

    if (_power_level_control != NULL)
    {
        delete _power_level_control;
        _power_level_control = NULL;
    }
}

void CChannelConfig::loop()
{
    _collar_comms.loop();

    if (_power_level_control != NULL)
        _power_level_control->loop();
}

CCollarComms* CChannelConfig::get_collar_comms()
{
    return &_collar_comms;
}

void CChannelConfig::configure_channels_from_saved_config(std::vector<COutputChannel*>* active_channels)
{
    struct routine_conf conf;
    populate_default_routine_chanels_from_config(&conf);
    configure_channels(active_channels, conf.channels);
}

void CChannelConfig::populate_default_routine_chanels_from_config(struct routine_conf *conf)
{
    for (uint8_t chanel_id = 0; chanel_id <= get_highest_enabled_channel_id(); chanel_id++)
    {
        CSavedSettings::channel_selection channel_details = g_SavedSettings->get_channel(chanel_id);
        channel_config_t chan;
        chan.index = channel_details.index;
        chan.type = channel_details.type;
        conf->channels.push_back(chan);
    }
}

uint8_t CChannelConfig::get_highest_enabled_channel_id()
{
    uint8_t highest_id = 0;
    for (int channel_id=0; channel_id < EEPROM_CHANNEL_COUNT; channel_id++)
    {
        CSavedSettings::channel_selection channel_details = _saved_settings->get_channel(channel_id);
        if (channel_details.type != CChannel_types::channel_type::CHANNEL_NONE)
            highest_id = channel_id;
    }

    return highest_id;
}

void CChannelConfig::configure_channels(std::vector<COutputChannel*>* active_channels, std::vector<channel_config_t>& chanel_conf)
{
    // delete any existing configured chanels
    for (size_t channel_id=0; channel_id < (*active_channels).size(); channel_id++)
    {
        delete (*active_channels)[channel_id];
    }
    (*active_channels).clear();

    if (_power_level_control != NULL)
    {
        _remote_access_mode_active = _power_level_control->is_remote_access_mode_active();
        delete _power_level_control;
    }

    _power_level_control = new CPowerLevelControl(_saved_settings, chanel_conf.size());
    if (_remote_access_mode_active)
        _power_level_control->remote_mode_enable();

    for (uint8_t channel_id=0; channel_id < chanel_conf.size(); channel_id++)
    {
        (*active_channels).push_back(get_ouput_chanel(chanel_conf[channel_id].type, chanel_conf[channel_id].index, channel_id));
    }

    printf("CChannelConfig::configure_channels: active_channels.size=%d, details:\n", (*active_channels).size());
    for (uint8_t idx=0; idx < (*active_channels).size(); idx++)
    {
        printf("\tchannel_id=%d is type %d\n", idx, (uint8_t)(*active_channels)[idx]->get_channel_type());
    }
}

COutputChannel* CChannelConfig::get_ouput_chanel(CChannel_types::channel_type channel_type, uint8_t channel_index, uint8_t channel_id)
{
    switch (channel_type)
    {
        case CChannel_types::channel_type::CHANNEL_COLLAR:
            return new CCollarChannel(_saved_settings, &_collar_comms, _power_level_control, channel_index, channel_id);

        case CChannel_types::channel_type::CHANNEL_INTERNAL:
            return new CZC624Channel(_saved_settings, &_zc624_comms, _power_level_control, channel_index, channel_id);

        case CChannel_types::channel_type::CHANNEL_NONE:
            return new CDummyOutput(_saved_settings, _power_level_control, channel_id);

        default:
            printf("ChannelConfig::get_ouput_chanel: Error - unexpected channel type encountered\n");
            return new CDummyOutput(_saved_settings, _power_level_control, channel_id);
    }
}

void CChannelConfig::clear_chanel_config(std::vector<COutputChannel*>* active_channels)
{
    for (size_t channel_id=0; channel_id < (*active_channels).size(); channel_id++)
    {
        delete (*active_channels)[channel_id];
    }
    (*active_channels).clear();

    if (_power_level_control != NULL)
    {
        _remote_access_mode_active = _power_level_control->is_remote_access_mode_active();
        delete _power_level_control;
        _power_level_control = NULL;
    }
}

void CChannelConfig::shutdown_zc624()
{
    printf("shutting down zc624 output module\n");
    CZC624Comms::message message = {0};
    message.command = (uint8_t)CZC624Comms::spi_command_t::PowerDown;
    _zc624_comms.send_message(message);
}

CPowerLevelControl* CChannelConfig::PowerLevelControl()
{
    return _power_level_control;
}
