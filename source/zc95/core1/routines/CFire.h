#include "CRoutine.h"

class CFire: public CRoutine
{
    public:
        CFire(uint8_t param);
        ~CFire();
        static CRoutine* create(uint8_t param) { return new CFire(param); };
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        void all_channels(bool on);
        void all_channels_pulse(uint16_t pulse_len_ms);
        int16_t _pulse_len_ms;
        bool _pulse_mode = false;
};
