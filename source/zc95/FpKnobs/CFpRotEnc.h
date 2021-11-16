#include "CFpKnobs.h"
#include "CRotEnc.h"
#include "../CSavedSettings.h"
#include "../config.h"

class CFpRotEnc : public CFpKnobs
{
    public:
        enum rot_encoder
        {
            ROT_ENCODER_CHANNEL1 = 0,
            ROT_ENCODER_CHANNEL2 = 1,
            ROT_ENCODER_CHANNEL3 = 2,
            ROT_ENCODER_CHANNEL4 = 3,
            ROT_ENCODER_ADJUST   = 4,
        };

        CFpRotEnc(CSavedSettings *saved_settings);
        void process(bool always_update);
        void interupt (port_exp exp); 
        uint16_t get_channel_power_level(uint8_t channel);
        int8_t get_adjust_control_change();

    private:
        uint8_t read_port_expander(port_exp exp);
        void write_port_expander(port_exp exp, uint8_t val);
        void rot_encoder_process(rot_encoder rot, uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos);
        void update_power_levels();
        void update_power_level(rot_encoder rot, uint8_t channel);
        void process_u1();
        void process_u2();

        CSavedSettings *_saved_settings;
        uint8_t _last_read[2];
        
        CRotEnc _rot_encoder[5];
        int16_t _power_level[MAX_CHANNELS];
        int16_t _adjust_value;
        volatile bool _interrupt_U1;
        volatile bool _interrupt_U2;
        
};

