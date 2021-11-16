#include "COutput312b.h"

/*
 * Allow the use of an mk312bt's outputs as channels.
 * TODO: probably remove this now. It's pointless now the ZC624 ouput board works
 */ 

COutput312b::COutput312b(CSavedSettings *saved_settings, C312bComms *comms, Channel c, CPowerLevelControl *power_level_control, uint8_t channel_id) : 
    CSimpleOutputChannel(saved_settings, power_level_control, channel_id)
{
    _comms = comms;
    _channel = c;
}

void COutput312b::on(int16_t power)
{
    // Power requested is 0-1000, but 312b expects 0-255, so scale
    int16_t tx_power_level = 0;
    if (power > 0)
    {
        float scaled_power = (float)power * 0.255;
        tx_power_level = ceil(scaled_power);
        if (tx_power_level > 255)
            tx_power_level = 255;
    } 

    if (_channel == Channel::CHAN_A)
    {
        _comms->send(ETMEM_knoba, (uint8_t)tx_power_level);
    }
    else if (_channel == Channel::CHAN_B)
    {
        _comms->send(ETMEM_knobb, (uint8_t)tx_power_level);
    }
}

void COutput312b::off()
{
    if (_channel == Channel::CHAN_A)
    {
        _comms->send(ETMEM_knoba, 0);
    }
    else if (_channel == Channel::CHAN_B)
    {
        _comms->send(ETMEM_knobb, 0);
    }
}
