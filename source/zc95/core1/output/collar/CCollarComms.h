#ifndef _CCOLLARCOMMS_H
#define _CCOLLARCOMMS_H

#include <stdint.h>
#include <queue>

#include "../CSimpleOutputChannel.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "collar433.pio.h"


class CCollarComms
{
  public:
    enum class collar_channel { CH1=0, CH2=1, CH3=2 };
    enum class collar_status { ON, OFF };
    enum collar_mode { SHOCK=1, VIBE=2, BEEP=3 };
    struct collar_message
    {
      uint16_t id;
      collar_mode mode;
      collar_channel channel;
      uint8_t power;
    };

    CCollarComms(uint8_t tx_pin);
    ~CCollarComms();
    void loop();
    bool transmit (uint16_t id, collar_channel channel, collar_mode mode, uint8_t power);
    bool transmit (struct collar_message message);
    static std::string mode_to_string(uint8_t mode);

  private:
    union msg_part
    {
        uint8_t byte[4];
        uint32_t data;
    };

    void tx_buffer(uint8_t *buf, uint8_t buf_len);
    bool is_pio_fifo_queue_full();
    PIO _pio;
    uint _sm;
    uint _program_offset;

    uint64_t _last_tx_time_us;
    std::queue<collar_message> _msg_queue;
};

#endif
