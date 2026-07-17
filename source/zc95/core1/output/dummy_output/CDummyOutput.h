#ifndef _CDUMMYOUTPUT_H
#define _CDUMMYOUTPUT_H


#include <stdint.h>
#include "../COutputChannel.h"
#include "../../../CSavedSettings.h"
#include "pico/stdlib.h"


class CDummyOutput : public COutputChannel
{
  public:
    CDummyOutput(CSavedSettings *saved_settings, CPowerLevelControl *power_level_control, uint8_t channel_id);
    ~CDummyOutput();
    void set_absolute_power(uint16_t power);
    void channel_pulse(uint16_t minimum_duration_ms);
    void off();
    void loop(uint64_t time_us);
    CChannel_types::channel_type get_channel_type();

  private:
    uint8_t _channel_id;
};

#endif
