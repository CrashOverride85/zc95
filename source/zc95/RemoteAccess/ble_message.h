#include <stdint.h>

struct ble_message_pulse_t
{
    uint8_t cmd_type;          // 1
    uint8_t pulse_width;       // 1
    uint16_t amplitude;        // 2
    uint64_t time_us;          // 8
    uint8_t channel_polarity;  // 1  // channel and polarity. For each chan:
                                     //   - 00 = no pulse
                                     //   - 01 = pos only pulse
                                     //   - 10 - neg only pulse
                                     //   - 11 - pos+neg pulse
} __attribute__((packed));     // => 13 bytes

#define BLE_MSG_PULSE_RESET 0x01
#define BLE_MSG_PULSE_PULSE 0x02
