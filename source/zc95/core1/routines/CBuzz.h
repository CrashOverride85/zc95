#include "CRoutine.h"

class CBuzz: public CRoutine
{
    public:
        CBuzz(uint8_t param);
        ~CBuzz();
        static CRoutine* create(uint8_t param) { return new CBuzz(param); };
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
        uint16_t _power_increment_period_ms;
        uint8_t  _shock_increment_pp;
        uint16_t _inital_shock_power_level;
        int16_t  _shock_len_ms;

        uint64_t _next_power_increase_us;
        uint16_t _current_power_level;

        uint64_t _last_shock_increment_us;
        uint16_t _shock_power_level;
        uint64_t _shock_start_time_us;
        uint64_t _shock_trigger_active;
        
        bool _game_running;
        uint64_t _end_game_at_us;

        void set_power_increment_ms_from_game_length(uint16_t game_len_sec);
        void stop_game();
        void start_game();
        void shock_start();
        void shock_stop();
        void shock_power_level(uint16_t new_power_level);
};
