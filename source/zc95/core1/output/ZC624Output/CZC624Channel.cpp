#include "CZC624Channel.h"
#include "../../../../common/zc95_config.h"
#include "../../../globals.h"

CZC624Channel::CZC624Channel(CSavedSettings *saved_settings, CZC624Comms *comms, CPowerLevelControl *power_level_control, uint8_t zc624_chan_index, uint8_t channel_id) : 
COutputChannel(saved_settings, power_level_control, channel_id) 
{
    printf("CZC624Channel(zc624=%d, channel_id=%d)\n", zc624_chan_index, channel_id);
    _comms = comms;
    _zc624_chan_index = zc624_chan_index;
    _channel_id = channel_id;
    _standby_led_colour = LedColour::Green;
    set_led_colour(_standby_led_colour);
}

CZC624Channel::~CZC624Channel()
{
    printf("~CZC624Channel(%d)\n", _zc624_chan_index);
    off();
    set_led_colour(LedColour::Black);
}

CChannel_types::channel_type CZC624Channel::get_channel_type()
{
    return CChannel_types::channel_type::CHANNEL_INTERNAL;
}

void CZC624Channel::channel_single_pulse(uint8_t pos_us, uint8_t neg_us)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::Pulse;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = pos_us;
    msg.arg2 = neg_us;

    _comms->send_message(msg);
}

void CZC624Channel::channel_pulse(uint16_t min_pulse_us)
{
    // TODO
}

void CZC624Channel::set_freq(uint16_t freq_hz)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetFreq;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = (freq_hz >> 8) & 0xFF;
    msg.arg2 = freq_hz & 0xFF;

    _comms->send_message(msg);
}

void CZC624Channel::set_pulse_width(uint8_t pulse_width_pos_us, uint8_t pulse_width_neg_us)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetPulseWidth;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = pulse_width_pos_us;
    msg.arg2 = pulse_width_neg_us;

    _comms->send_message(msg);
}

void CZC624Channel::on()
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SwitchOn;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = 0;
    msg.arg2 = 0;

    _comms->send_message(msg);
}

void CZC624Channel::off()
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SwitchOff;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = 0;
    msg.arg2 = 0;

    _comms->send_message(msg);
}

void CZC624Channel::link_channel(uint8_t channel, uint8_t offset_percentage)
{
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SyncChanel;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = channel;
    msg.arg2 = offset_percentage;

    _comms->send_message(msg);
}

void CZC624Channel::set_absolute_power(uint16_t power)
{
    // The ZC624 output module also expects power levels 0-1000, so no scaling required
    CZC624Comms::message msg;

    msg.command = (uint8_t)CZC624Comms::spi_command_t::SetPower;
    msg.arg0 = _zc624_chan_index;
    msg.arg1 = (power >> 8) & 0xFF;
    msg.arg2 = power & 0xFF;

    _comms->send_message(msg);
}

void CZC624Channel::loop(uint64_t time_us)
{    
    bool new_led_state = _comms->loop(_zc624_chan_index);
    
    // Send LED update to core0 whenever the state changes, or every 500ms if no update. This means that 
    // if an update gets lost (e.g. due to full FIFO queue), "stuck" LEDs get fixed after at most 500 ms
    if (new_led_state != _last_led_state || time_us_64() - _last_led_update_us > (1000 * 500))
    {
        if (new_led_state)
            set_led_colour(LedColour::Red);
        else
            set_led_colour(_standby_led_colour);

        _last_led_state = new_led_state;
        _last_led_update_us = time_us_64();
    }
}

// Enable/disable channel isolation. This setting does have some safety implications, so try and confirm 
// that it does get set, and shutdown box if it doesn't.
bool CZC624Channel::set_channel_isolation(bool on)
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

    if (gZc624ChannelIsolationEnabled != on)
    {
        if (on)
            printf("CZC624Channel::set_channel_isolation: Channel isolation enabled\n");
        else
            printf("CZC624Channel::set_channel_isolation: Channel isolation DISABLED\n");

        gZc624ChannelIsolationEnabled = on;
    }
    return true;
}
