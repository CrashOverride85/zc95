#ifndef _AUDIOTYPES_H
#define _AUDIOTYPES_H

enum class audio_mode_t
{
    OFF,
    THRESHOLD_CROSS_FFT,
    AUDIO3,
    AUDIO_INTENSITY,
    AUDIO_VIRTUAL_3
};

enum class audio_hardware_state_t
{
    NOT_PRESENT,
    PRESENT_NO_GAIN,
    PRESENT
};

enum audio_channel_t
{
    AUDIO_LEFT  = 0,
    AUDIO_RIGHT = 1,
    AUDIO_VIRT  = 2
};

#endif
