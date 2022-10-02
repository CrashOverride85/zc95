#include "CZC624ChannelFull.h"
#include "../../../config.h"
#include "../../../globals.h"

CZC624ChannelFull::CZC624ChannelFull(CSavedSettings *saved_settings, CZC624Comms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id) : 
CFullOutputChannel(saved_settings, power_level_control, channel_id) 
{
    printf("CZC624ChannelFull(%d)\n", channel_id);
    _comms = comms;
    _channel_id = channel_id;
    _inital_led_colour = LedColour::Green;
    _led_off_time = 0;
    set_led_colour(_inital_led_colour);
}

CZC624ChannelFull::~CZC624ChannelFull()
{
    printf("~CZC624ChannelFull(%d)\n", _channel_id);
    off();
    set_led_colour(LedColour::Black);
}

void CZC624ChannelFull::channel_pulse(uint8_t pos_us, uint8_t neg_us)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::Pulse;
    msg.arg0 = _channel_id;
    msg.arg1 = pos_us;
    msg.arg2 = neg_us;

    set_led_colour(LedColour::Red);
    _led_off_time = get_time_us() + 10000;
    _comms->send_message(msg);
}

void CZC624ChannelFull::set_freq(uint16_t freq_hz)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetFreq;
    msg.arg0 = _channel_id;
    msg.arg1 = (freq_hz >> 8) & 0xFF;
    msg.arg2 = freq_hz & 0xFF;

    _comms->send_message(msg);
}

void CZC624ChannelFull::set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetPulseWitdh;
    msg.arg0 = _channel_id;
    msg.arg1 = pulse_width_pos_us;
    msg.arg2 = pulse_width_neg_us;

    _comms->send_message(msg);
}

void CZC624ChannelFull::on()
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SwitchOn;
    msg.arg0 = _channel_id;
    msg.arg1 = 0;
    msg.arg2 = 0;

    _comms->send_message(msg);
    set_led_colour(LedColour::Red);
}

void CZC624ChannelFull::off()
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SwitchOff;
    msg.arg0 = _channel_id;
    msg.arg1 = 0;
    msg.arg2 = 0;

    _comms->send_message(msg);
    set_led_colour(LedColour::Green);
}

void CZC624ChannelFull::set_absolute_power(uint16_t power)
{
    // The ZC624 output module also expects power levels 0-1000, so no scaling required
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetPower;
    msg.arg0 = _channel_id;
    msg.arg1 = (power >> 8) & 0xFF;
    msg.arg2 = power & 0xFF;

    _comms->send_message(msg);
}

void CZC624ChannelFull::loop(uint64_t time_us)
{
    if (_led_off_time && time_us > _led_off_time)
    {
        set_led_colour(LedColour::Green);
        _led_off_time = 0;
    }
}

bool CZC624ChannelFull::is_internal()
{
    return true;
}

// Enable/disable channel isolation. This setting does have some saftey implications, so try and confirm 
// that it does get set, and shutdown box if it doesn't.
bool CZC624ChannelFull::set_channel_isolation(bool on)
{
    bool success = _comms->write_i2c_register(CZC624Comms::i2c_reg_t::ChannelIsolation, on);
    if (!success)
    {
        printf("failed to set channel isolation! (write)\n");
        gErrorString = "Comms error with ZC624\noutput module.\nError setting Channel\nIsolation.";
        gFatalError = true;
        return false;
    }

    // Read back value to double check it got set ok
    uint8_t value = 0xFF;
    success = _comms->get_i2c_register(CZC624Comms::i2c_reg_t::ChannelIsolation, &value);
    if (!success || (value != on))
    {
        printf("failed to set channel isolation! (read back: success=%d, value=%d)\n", success, value);
        gErrorString = "Comms error with ZC624\noutput module.\nError getting Channel\nIsolation.";
        gFatalError = true;
        return false;
    }

    return true;
}
