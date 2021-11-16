#ifndef _CCOLLARCHANNEL_H
#define _CCOLLARCHANNEL_H


#include <stdint.h>
#include "CCollarComms.h"
#include "../CSimpleOutputChannel.h"
#include "../../../CSavedSettings.h"
#include "pico/stdlib.h"


class CCollarChannel : public CSimpleOutputChannel
{
  public:
    enum class collar_status { ON, OFF };

    CCollarChannel(CSavedSettings *saved_settings, CCollarComms *comms, CPowerLevelControl *power_level_control, uint8_t channel_id);
    ~CCollarChannel();
    void on();
    void set_absolute_power(uint16_t power);
    void pulse(uint16_t minimum_duration_ms);
    void off();
    void loop(uint64_t time_us);

  private:
    void set_collar_level_from_power(int16_t power);
    void transmit (uint8_t power);
    CCollarComms *_comms;
    struct CSavedSettings::collar_config _collar_conf;

    uint8_t _channel_id;
    uint64_t _last_tx_time_us;
    collar_status _current_status;
    uint8_t _collar_level;
    uint64_t _pulse_end_time;
    uint64_t _led_off_time;
};

#endif
