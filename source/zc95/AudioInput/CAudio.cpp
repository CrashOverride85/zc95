#include "CAudio.h"



CAudio::CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls)
{
    _analogueCapture = analogueCapture;
    _mcp4651 = mcp4651;
    _controlsPortExp = controls;
}

void CAudio::get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer)
{
    _analogueCapture->get_audio_buffer(chan, samples, buffer);
}

void CAudio::set_gain(CAnalogueCapture::channel chan, uint8_t value)
{
    if (chan == CAnalogueCapture::channel::LEFT)
        _mcp4651->set_val(0, 255-value);
    else
        _mcp4651->set_val(0, 255-value);
}

void CAudio::mic_preamp_enable(bool enable)
{
    _controlsPortExp->mic_preamp_enable(enable);
}

void CAudio::mic_power_enable(bool enable)
{
    _controlsPortExp->mic_power_enable(enable);
}
