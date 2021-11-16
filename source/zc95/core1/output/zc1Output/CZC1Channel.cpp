#include "CZC1Channel.h"
#include "../../../config.h"

CZC1Channel::CZC1Channel(CSavedSettings *saved_settings, CZC1Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id) :
CSimpleOutputChannel(saved_settings, power_level_control, channel_id) 
{
    printf("CZC1Channel(%d)\n", channel_id);
    _comms = comms;
    _channel_id = channel_id;
    _inital_led_colour = LedColour::Green;
    _on = false;
    _interval_ms = (1 / DEFAULT_FREQ_HZ) * 1000;
    _next_pulse_time = 0;
    _off_time = 0;
    set_led_colour(_inital_led_colour);
}

CZC1Channel::~CZC1Channel()
{
    printf("~CZC1Channel(%d)\n", _channel_id);
    set_led_colour(LedColour::Black);
}

// 

//////////// CSimpleOutputChannel stuff ////////////

void CZC1Channel::on()
{
    set_led_colour(LedColour::Red);
    _next_pulse_time = 0;
    _on = true;
    _off_time = 0;
}

void CZC1Channel::off()
{
    set_led_colour(LedColour::Green);
    _on = false;
    _off_time = 0;
}

void CZC1Channel::pulse(uint16_t minimum_duration_ms)
{
    on();
    _off_time = get_time_us() + (minimum_duration_ms * 1000);
}

////////////////////////////////////////////////////

void CZC1Channel::set_power(uint16_t power)
{
    // The ZC1 output module also expects power levels 0-1000, so no scaling required
    CZC1Comms::message msg;

    msg.command = (uint8_t)CZC1Comms::command::SetPower;
    msg.arg0 = _channel_id;
    msg.arg1 = (power >> 8) & 0xFF;
    msg.arg2 = power & 0xFF;

    _comms->send_message(msg);
}

void CZC1Channel::loop(uint64_t time_us)
{
    if (_on)
    {
        if (_off_time && (time_us > _off_time))
        {
            off();
        }
        else
        {
            if (time_us > _next_pulse_time)
            {
                _next_pulse_time = time_us + (_interval_ms * 1000);
                send_pulse_message(DEFAULT_PULSE_WIDTH);
            }
        }
    }
}

void CZC1Channel::send_pulse_message(uint8_t pulse_width)
{
    CZC1Comms::message msg;

    msg.command = (uint8_t)CZC1Comms::command::Pulse;
    msg.arg0 = _channel_id;
    msg.arg1 = pulse_width;
    msg.arg2 = pulse_width;

    _comms->send_message(msg);
}

// DEFAULT_PULSE_WIDTH
bool CZC1Channel::is_internal()
{
    return true;
}
