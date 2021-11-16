#include "CFpKnobs.h"
#include "CRotEnc.h"
#include "../CSavedSettings.h"
#include "../config.h"

class CFpAnalog : public CFpKnobs
{
    public:
        CFpAnalog(CSavedSettings *saved_settings);
        void process(bool always_update);
        uint16_t get_channel_power_level(uint8_t channel);
        int8_t get_adjust_control_change();
        void interupt (port_exp exp);

    private:
        void read_adc();
        void rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos);
        uint8_t read_port_expander();
        uint8_t _last_port_exp_read;
        int16_t _power_level[MAX_CHANNELS];
        CRotEnc _rot_encoder;
        int16_t _adjust_value;
        volatile bool _interrupt;
};
