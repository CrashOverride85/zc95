#include "CRoutine.h"

class CTriggeredClimb: public CRoutine
{
    public:
        CTriggeredClimb(uint8_t param);
        ~CTriggeredClimb();
        static CRoutine* create(uint8_t param) { return new CTriggeredClimb(param); }
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void soft_button_pushed (soft_button button, bool pushed);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        void reset();
        int16_t _shock_inc_by = 0;
        int16_t _shock_duration_ms = 0;


        int16_t _climbing_power_level = 0;
        int16_t _shock_power_level = 0;
        uint16_t _power_increment_period_ms;


        uint64_t _next_increment_power_at_us;
        
        bool _shock_required;
        uint64_t _shock_end_us;

        int16_t _speed_setting;
        uint64_t _wait_until_us;
        uint8_t _current_active_channel;
        bool _pulse_mode = false;
};
