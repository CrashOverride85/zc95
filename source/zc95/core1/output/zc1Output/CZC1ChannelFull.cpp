#include "CZC1ChannelFull.h"
#include "../../../config.h"

CZC1ChannelFull::CZC1ChannelFull(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id) : 
CFullOutputChannel(saved_settings, power_level_control, channel_id) 
{
    printf("CZC1ChannelFull(%d)\n", channel_id);
    _comms = comms;
    _channel_id = channel_id;
    _inital_led_colour = LedColour::Green;
    _led_off_time = 0;
    set_led_colour(_inital_led_colour);
}

CZC1ChannelFull::~CZC1ChannelFull()
{
    printf("~CZC1ChannelFull(%d)\n", _channel_id);
    set_led_colour(LedColour::Black);
}

void CZC1ChannelFull::channel_pulse(uint8_t pos_us, uint8_t neg_us)
{
    CZC1Comms::message msg;

    msg.command = (uint8_t)CZC1Comms::command::Pulse;
    msg.arg0 = _channel_id;
    msg.arg1 = pos_us;
    msg.arg2 = neg_us;

    set_led_colour(LedColour::Red);
    _led_off_time = get_time_us() + 10000;
    _comms->send_message(msg);
}

void CZC1ChannelFull::set_absolute_power(uint16_t power)
{
    // The ZC1 output module also expects power levels 0-1000, so no scaling required
    CZC1Comms::message msg;

    msg.command = (uint8_t)CZC1Comms::command::SetPower;
    msg.arg0 = _channel_id;
    msg.arg1 = (power >> 8) & 0xFF;
    msg.arg2 = power & 0xFF;

    _comms->send_message(msg);
}

void CZC1ChannelFull::loop(uint64_t time_us)
{
    if (_led_off_time && time_us > _led_off_time)
    {
        set_led_colour(LedColour::Green);
        _led_off_time = 0;
    }
}

bool CZC1ChannelFull::is_internal()
{
    return true;
}
