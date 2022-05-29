#include "CZC1ChannelSimple.h"
#include "../../../config.h"

CZC1ChannelSimple::CZC1ChannelSimple(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id) :
CSimpleOutputChannel(saved_settings, power_level_control, channel_id) 
{
    printf("CZC1Channel(%d)\n", channel_id);
    _comms = comms;
    _channel_id = channel_id;
    _inital_led_colour = LedColour::Green;
    _on = false;
    _off_time = 0;
    set_led_colour(_inital_led_colour);
}

CZC1ChannelSimple::~CZC1ChannelSimple()
{
    printf("~CZC1Channel(%d)\n", _channel_id);
    set_led_colour(LedColour::Black);
}

void CZC1ChannelSimple::on()
{
    set_led_colour(LedColour::Red);
    _on = true;
    _off_time = 0;
}

void CZC1ChannelSimple::off()
{
    set_led_colour(LedColour::Green);
    _on = false;
    _off_time = 0;
}

void CZC1ChannelSimple::pulse(uint16_t minimum_duration_ms)
{
    on();
    _off_time = get_time_us() + (minimum_duration_ms * 1000);
}

////////////////////////////////////////////////////

void CZC1ChannelSimple::set_power(uint16_t power)
{
    // The ZC1 output module also expects power levels 0-1000, so no scaling required
    CZC1Comms::message msg;

    msg.command = (uint8_t)CZC1Comms::command::SetPower;
    msg.arg0 = _channel_id;
    msg.arg1 = (power >> 8) & 0xFF;
    msg.arg2 = power & 0xFF;

    _comms->send_message(msg);
    printf("CZC1Channel::set_power(%d)\n", power);
}

void CZC1ChannelSimple::loop(uint64_t time_us)
{
    if (_on)
    {
        if (_off_time && (time_us > _off_time))
        {
            off();
        }
    }
}

bool CZC1ChannelSimple::is_internal()
{
    return true;
}
