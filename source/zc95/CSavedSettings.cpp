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

#include "CSavedSettings.h"
#include "string.h"
#include "config.h"
#include "core1/output/collar/CCollarComms.h"
#include "CHwCheck.h"

/*
 * Manage access and updating of settings saved to EEPROM.
 * Assumes 512byte EEPROM, and loads its contents into memory.
 * A blank EEPROM should be automatically initialized with default values on first 
 * run; a blank EEPROM is detected by the absence of EEPROM_MAGIC_VAL in location 0.
 */

CSavedSettings::CSavedSettings(CEeprom *eeprom)
{
    _eeprom = eeprom;
    memset(_eeprom_contents, 0, sizeof(_eeprom_contents));

    if (!eeprom_initialised())
    {
        eeprom_initialise();
    }

    printf("Read eeprom...");
    for (int n=0; n < EEPROM_SIZE; n++)
        _eeprom_contents[n] = _eeprom->read(n);
    printf("done\n");
}

CSavedSettings::~CSavedSettings()
{

}

void CSavedSettings::save()
{
    printf("Save changed settings to EEPROM...");
    for (int n=0; n < EEPROM_SIZE; n++)
    {
        if (_eeprom->read(n) != _eeprom_contents[n])
        {
            _eeprom->write(n, _eeprom_contents[n], true);
        }
    }
    printf("done\n");
}

CSavedSettings::channel_selection CSavedSettings::get_channel(uint8_t channel_id)
{
    channel_selection ret;
    
    if (channel_id < EEPROM_CHANNEL_COUNT)
    {
        ret.type  = (CChannel_types::channel_type)_eeprom_contents[(uint8_t)(setting::ChannelType) + (channel_id*2)];
        ret.index = _eeprom_contents[(uint8_t)(setting::ChannelIndex) + (channel_id*2)];
    }

    return ret;
}

void CSavedSettings::set_channel(channel_selection chan, uint8_t channel_id)
{
    if (channel_id < EEPROM_CHANNEL_COUNT)
    {
        _eeprom_contents[(uint8_t)setting::ChannelType  + (channel_id*2)] = (uint8_t)chan.type;
        _eeprom_contents[(uint8_t)setting::ChannelIndex + (channel_id*2)] = chan.index;
    }
}

uint8_t CSavedSettings::get_led_brightness()
{
    return _eeprom_contents[(uint8_t)setting::LEDBrightness];
}

void CSavedSettings::set_led_brightness(uint8_t led_brightness_percent)
{
    _eeprom_contents[(uint8_t)setting::LEDBrightness] = led_brightness_percent;
}

uint8_t CSavedSettings::get_power_step_interval()
{
    return _eeprom_contents[(uint8_t)setting::PowerStep];
}

void CSavedSettings::set_power_step_interval(uint8_t power_step)
{
    _eeprom_contents[(uint8_t)setting::PowerStep] = power_step;
}

uint8_t CSavedSettings::get_ramp_up_time_seconds()
{
    return _eeprom_contents[(uint8_t)setting::RampUpTimeSecs];
}

void CSavedSettings::set_ramp_up_time_seconds(uint8_t time_secs)
{
    _eeprom_contents[(uint8_t)setting::RampUpTimeSecs] = time_secs;
}

uint8_t CSavedSettings::get_audio_gain_left()
{
    return _eeprom_contents[(uint8_t)setting::AudioGainL];
}

uint8_t CSavedSettings::get_audio_gain_right()
{
    return _eeprom_contents[(uint8_t)setting::AudioGainR];
}

void CSavedSettings::set_audio_gain_left(uint8_t gain)
{
    _eeprom_contents[(uint8_t)setting::AudioGainL] = gain;
}

void CSavedSettings::set_audio_gain_right(uint8_t gain)
{
    _eeprom_contents[(uint8_t)setting::AudioGainR] = gain;
}

bool CSavedSettings::get_mic_preamp_enabled()
{
    return _eeprom_contents[(uint8_t)setting::MicPreAmp] > 0;
}

void CSavedSettings::set_mic_preamp_enabled(bool enabled)
{
    _eeprom_contents[(uint8_t)setting::MicPreAmp] = enabled ? 1 : 0;
}

bool CSavedSettings::get_mic_power_enabled()
{
    return _eeprom_contents[(uint8_t)setting::MicPower] > 0;
}

void CSavedSettings::set_mic_power_enabled(bool enabled)
{
    _eeprom_contents[(uint8_t)setting::MicPower] = enabled ? 1 : 0;
}

CSavedSettings::setting_audio CSavedSettings::get_audio_setting()
{
    return (CSavedSettings::setting_audio)_eeprom_contents[(uint8_t)setting::Audio];
}

void CSavedSettings::set_audio_setting(setting_audio setting)
{
    _eeprom_contents[(uint8_t)setting::Audio] = (uint8_t)setting;
}

CSavedSettings::setting_debug CSavedSettings::get_debug_dest()
{
return (CSavedSettings::setting_debug)_eeprom_contents[(uint8_t)setting::Debug];
}

void CSavedSettings::set_debug_dest(setting_debug setting)
{
    _eeprom_contents[(uint8_t)setting::Debug] = (uint8_t)setting;
}

CSavedSettings::setting_aux_port_use CSavedSettings::get_aux_port_use()
{
    return (CSavedSettings::setting_aux_port_use)_eeprom_contents[(uint8_t)setting::AuxPort];
}

void CSavedSettings::set_aux_port_use(setting_aux_port_use setting)
{
    _eeprom_contents[(uint8_t)setting::AuxPort] = (uint8_t)setting;
}

bool CSavedSettings::get_wifi_credentials(std::string &out_ssid, std::string &out_password)
{
    // If wifi not yet configured, don't try to get ssid/password
    if (_eeprom_contents[(uint8_t)setting::WiFiConfigured] != EEPROM_MAGIC_VAL)
        return false;

    // If the last character of the ssid isn't a NULL, somethings gone wrong (WifiPSK is the next thing after WifiSSID)
    if (_eeprom_contents[(uint8_t)setting::WifiPSK - 1] != '\0')
        return false;
    
    // If the last character of the password isn't a NULL, somethings gone wrong (WiFiConfigured is the next thing after WifiPSK)
    if (_eeprom_contents[(uint8_t)setting::WiFiConfigured - 1] != '\0')
        return false;
    
    out_ssid = (char*)&_eeprom_contents[(uint8_t)setting::WifiSSID];
    out_password = (char*)&_eeprom_contents[(uint8_t)setting::WifiPSK];

    return true;
}

bool CSavedSettings::set_wifi_credentials(std::string ssid, std::string password)
{
    if (ssid.length() > (uint8_t)setting::WifiPSK - (uint8_t)setting::WifiSSID - 1)
    {
        printf("ssid [%s] too long, not saving\n", ssid.c_str());
        return false;
    }

    if (ssid.length() > (uint8_t)setting::WiFiConfigured - (uint8_t)setting::WifiPSK - 1)
    {
        printf("password [%s] too long, not saving\n", password.c_str());
        return false;
    }

    strcpy((char*)&_eeprom_contents[(uint8_t)setting::WifiSSID], ssid.c_str()    );
    strcpy((char*)&_eeprom_contents[(uint8_t)setting::WifiPSK] , password.c_str());
    _eeprom_contents[(uint8_t)setting::WiFiConfigured] = EEPROM_MAGIC_VAL;

    return true;
}

void CSavedSettings::clear_wifi_credentials()
{
    memset(&_eeprom_contents[(uint8_t)setting::WifiSSID], 0, (uint8_t)setting::WifiPSK - (uint8_t)setting::WifiSSID);
    _eeprom_contents[(uint8_t)setting::WiFiConfigured] = 0;
}

bool CSavedSettings::wifi_is_configured()
{
    return (_eeprom_contents[(uint8_t)setting::WiFiConfigured] == EEPROM_MAGIC_VAL);
}

bool CSavedSettings::get_wifi_ap_psk(std::string &out_psk)
{
    out_psk = "";
    // If the last character of the psk isn't a NULL, somethings gone wrong
    if (_eeprom_contents[(uint8_t)setting::WifiApPskEnd] != '\0')
        return false;

    out_psk = (char*)&_eeprom_contents[(uint8_t)setting::WifiApPsk];
    
    // A psk length < 8 is invalid and won't work
    return (out_psk.length() >= 8);    
}

bool CSavedSettings::set_wifi_ap_psk(std::string psk)
{
    if (psk.length() < 8)
    {
        printf("set_wifi_ap_psk(): psk of [%s] is too short (must be 8-16 characters)\n", psk.c_str());
        return false;
    }

    if (psk.length() > 16)
    {
        printf("set_wifi_ap_psk(): psk of [%s] is too long (must be 8-16 characters)\n", psk.c_str());
        return false;
    }

    clear_saved_ap_psk();
    strcpy((char*)&_eeprom_contents[(uint8_t)setting::WifiApPsk], psk.c_str());

    return true;
}

void CSavedSettings::clear_saved_ap_psk()
{
    memset(&_eeprom_contents[(uint8_t)setting::WifiApPsk], 0, ((uint8_t)setting::WifiApPskEnd - (uint8_t)setting::WifiApPsk) + 1);
}

CSavedSettings::power_level_show_percent CSavedSettings::get_power_level_display()
{
    return (CSavedSettings::power_level_show_percent)_eeprom_contents[(uint8_t)setting::PowerLevelDisp];
}

bool CSavedSettings::power_level_show_in_bar_graph()
{
    return ((_eeprom_contents[(uint8_t)setting::PowerLevelDisp] == (uint8_t)power_level_show_percent::IN_BAR_GRAPH) ||
            (_eeprom_contents[(uint8_t)setting::PowerLevelDisp] == (uint8_t)power_level_show_percent::BOTH));
}

bool CSavedSettings::power_level_show_disappearing_text()
{
    return ((_eeprom_contents[(uint8_t)setting::PowerLevelDisp] == (uint8_t)power_level_show_percent::DISAPPEARING_TEXT) ||
            (_eeprom_contents[(uint8_t)setting::PowerLevelDisp] == (uint8_t)power_level_show_percent::BOTH));
}

void CSavedSettings::set_power_level_display(power_level_show_percent setting)
{
    _eeprom_contents[(uint8_t)setting::PowerLevelDisp] = (uint8_t)setting;
}

uint8_t CSavedSettings::get_button_brightness()
{
    return _eeprom_contents[(uint8_t)setting::ButtonLedBright];
}

void CSavedSettings::set_button_brightness(uint8_t button_brightness_byte)
{
    _eeprom_contents[(uint8_t)setting::ButtonLedBright] = button_brightness_byte;
}

bool CSavedSettings::get_collar_config(uint8_t collar_id, struct collar_config &collar_conf)
{
    if (collar_id > 9)
        return false;

    collar_conf.channel = _eeprom_contents[(uint8_t)setting::Collar0Chan + (collar_id*5)];
    collar_conf.mode    = _eeprom_contents[(uint8_t)setting::Collar0Mode + (collar_id*5)];

    collar_conf.id = _eeprom_contents[(uint8_t)setting::Collar0IdLow  + (collar_id*5)] |
                    (_eeprom_contents[(uint8_t)setting::Collar0IdHigh + (collar_id*5)] << 8);
    return true;
}

bool CSavedSettings::set_collar_config(uint8_t collar_id, struct collar_config &collar_conf)
{
    if (collar_id > 9)
        return false;

    _eeprom_contents[(uint8_t)setting::Collar0Chan + (collar_id*5)] = collar_conf.channel;
    _eeprom_contents[(uint8_t)setting::Collar0Mode + (collar_id*5)] = collar_conf.mode;

    _eeprom_contents[(uint8_t)setting::Collar0IdLow  + (collar_id*5)] = collar_conf.id & 0xFF;
    _eeprom_contents[(uint8_t)setting::Collar0IdHigh + (collar_id*5)] = collar_conf.id >> 8;

    return true;
}

bool CSavedSettings::get_bluethooth_enabled()
{
    return (_eeprom_contents[(uint8_t)setting::BluetoothOn] != 0) && CHwCheck::running_on_picow();
}

void CSavedSettings::set_bluethooth_enabled(bool setting)
{
    _eeprom_contents[(uint8_t)setting::BluetoothOn] = setting;
}

void CSavedSettings::get_paired_bt_address(bd_addr_t *address)
{
    if (sizeof(bd_addr_t) != ((uint8_t)setting::BTAddrEnd+1) - (uint8_t)setting::BTAddrStart) // +1 because range is start->end *inclusive*
    {
        // this should never happen!
        printf("get_paired_bt_address: BUG: bd_addr_t size does not match space allocated in eeprom!\n");
        return;
    }

    memcpy(address, &_eeprom_contents[(uint8_t)setting::BTAddrStart], sizeof(bd_addr_t));
}

void CSavedSettings::set_paired_bt_address(bd_addr_t address)
{
    if (sizeof(bd_addr_t) != ((uint8_t)setting::BTAddrEnd+1) - (uint8_t)setting::BTAddrStart) // +1 because range is start->end *inclusive*
    {
        // this should never happen!
        printf("set_paired_bt_address: BUG: bd_addr_t size does not match space allocated in eeprom!\n");
        return;
    }

    memcpy(&_eeprom_contents[(uint8_t)setting::BTAddrStart], address, sizeof(bd_addr_t));
}

CBluetoothRemote::keypress_action_t CSavedSettings::get_bt_keypress_action(CBluetoothRemote::keypress_t key)
{
    uint8_t action = 0;

    switch(key)
    {
        case CBluetoothRemote::keypress_t::KEY_BUTTON:
            action = _eeprom_contents[(uint8_t)setting::BTButtonAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_UP:
            action = _eeprom_contents[(uint8_t)setting::BTUpAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_DOWN:
            action = _eeprom_contents[(uint8_t)setting::BTDownAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_LEFT:
            action = _eeprom_contents[(uint8_t)setting::BTLeftAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_RIGHT:
            action = _eeprom_contents[(uint8_t)setting::BTRightAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_SHUTTER:
            action = _eeprom_contents[(uint8_t)setting::BTShutterAction];
            break;

        case CBluetoothRemote::keypress_t::KEY_UNKNOWN:
            action = _eeprom_contents[(uint8_t)setting::BTUnknownAction];
            break;

        default:
            printf("get_bt_keypress_action: Error - request for unknown keypress type [0x%X]\n", (int)key);
            action = (uint8_t)CBluetoothRemote::keypress_action_t::NONE;
            break;
    }

    return (CBluetoothRemote::keypress_action_t)action;
}

void CSavedSettings::set_bt_keypress_action(CBluetoothRemote::keypress_t key, CBluetoothRemote::keypress_action_t action)
{
    switch(key)
    {
        case CBluetoothRemote::keypress_t::KEY_BUTTON:
            _eeprom_contents[(uint8_t)setting::BTButtonAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_UP:
            _eeprom_contents[(uint8_t)setting::BTUpAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_DOWN:
            _eeprom_contents[(uint8_t)setting::BTDownAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_LEFT:
            _eeprom_contents[(uint8_t)setting::BTLeftAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_RIGHT:
            _eeprom_contents[(uint8_t)setting::BTRightAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_SHUTTER:
            _eeprom_contents[(uint8_t)setting::BTShutterAction] = action;
            break;

        case CBluetoothRemote::keypress_t::KEY_UNKNOWN:
            _eeprom_contents[(uint8_t)setting::BTUnknownAction] = action;
            break;

        default:
            printf("set_bt_keypress_action: Error - unknown keypress type [0x%X]\n", (int)key);
            break;
    }
}

bool CSavedSettings::eeprom_initialised()
{
    return (_eeprom->read((uint16_t)setting::EepromInit) == EEPROM_MAGIC_VAL);
}

// Wipe the EEPROM and put it into a default state
void CSavedSettings::eeprom_initialise()
{
    printf("Initialise EEPROM...\n");
    memset(_eeprom_contents, 0, sizeof(_eeprom_contents));
    _eeprom_contents[(uint16_t)setting::EepromInit] = 0; // write this separately & last, in case something goes wrong mid way

    // Set channels 0->internal Chan1, 1->internal chan2, etc..., and the rest to none
    for (int channel_id=0; channel_id < 4; channel_id++)
    {
        _eeprom_contents[(uint8_t)setting::ChannelType  + (channel_id*2)] = (uint8_t)CChannel_types::channel_type::CHANNEL_INTERNAL;
        _eeprom_contents[(uint8_t)setting::ChannelIndex + (channel_id*2)] = channel_id;
    }

    // Set any remaining channels to nothing
    for (int channel_id=4; channel_id < EEPROM_CHANNEL_COUNT; channel_id++)
    {
        _eeprom_contents[(uint8_t)setting::ChannelType  + (channel_id*2)] = (uint8_t)CChannel_types::channel_type::CHANNEL_NONE;
        _eeprom_contents[(uint8_t)setting::ChannelIndex + (channel_id*2)] = 0;
    }

    // Default LED brightness to 10
    _eeprom_contents[(uint8_t)setting::LEDBrightness] = 10;

    // Default power level step to 10, giving 100 power levels. TODO: Remove. no longer applicable with POTs for power adjustment, not rotary encoders.
    _eeprom_contents[(uint8_t)setting::PowerStep] = 10;

    // Ramp up time - 5 secs seems like a reasonable default
    _eeprom_contents[(uint8_t)setting::RampUpTimeSecs] = 5;

    // Gain - put it somewhere near the middle. Note that the config screen changes
    // it in 5 step increments, so it's best if the default is a multiple of 5
    _eeprom_contents[(uint8_t)setting::AudioGainL] = 130;
    _eeprom_contents[(uint8_t)setting::AudioGainR] = 130;

    // Default microphone power and preamp to off - i.e. default is line level input
    _eeprom_contents[(uint8_t)setting::MicPower]  = 0;
    _eeprom_contents[(uint8_t)setting::MicPreAmp] = 0;

    _eeprom_contents[(uint8_t)setting::Audio]  = (uint8_t)setting_audio::AUTO;
    _eeprom_contents[(uint8_t)setting::Debug]  = (uint8_t)setting_debug::ACC_PORT;

    _eeprom_contents[(uint8_t)setting::PowerLevelDisp]  = (uint8_t)power_level_show_percent::OFF;

    for (uint8_t collar_id = 0; collar_id < EEPROM_CHANNEL_COUNT; collar_id++)
        initialise_collar(collar_id);

    _eeprom_contents[(uint8_t)setting::ButtonLedBright] = 10;

    // Save changes
    save();

    // Now set a flag in the EEPROM so we know it's been initialised
    _eeprom_contents[(uint16_t)setting::EepromInit] = EEPROM_MAGIC_VAL;
    save();
}

void CSavedSettings::initialise_collar(uint8_t collar_id)
{
    _eeprom_contents[(uint8_t)setting::Collar0Chan + (collar_id*5)] = (uint8_t)CCollarComms::collar_channel::CH1;
    _eeprom_contents[(uint8_t)setting::Collar0Mode + (collar_id*5)] = (uint8_t)CCollarComms::collar_mode::VIBE;

    _eeprom_contents[(uint8_t)setting::Collar0IdLow  + (collar_id*5)] = rand() & 0xFF;
    _eeprom_contents[(uint8_t)setting::Collar0IdHigh + (collar_id*5)] = rand() & 0xFF;
}
