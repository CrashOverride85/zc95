#include "CRoutine.h"

class CDirectPulse: public CRoutine
{
    public:
        CDirectPulse(uint8_t param);
        ~CDirectPulse();
        static CRoutine* create(uint8_t param) { return new CDirectPulse(param); };
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void pulse_message(uint8_t channel, uint16_t power_level, uint8_t pos_pulse_us, uint8_t neg_pulse_us);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        uint16_t _chan_last_power_level[MAX_CHANNELS] = {0};
        uint8_t _chan_pos_pulse_width[MAX_CHANNELS];
        uint8_t _chan_neg_pulse_width[MAX_CHANNELS];
};
