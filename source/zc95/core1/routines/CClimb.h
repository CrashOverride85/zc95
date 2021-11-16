#include "CRoutine.h"



class CClimb: public CRoutine
{
    public:
        CClimb();
        ~CClimb();
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        const int64_t _inital_pulse_gap_us = 60000;
        void set_pulse_step_from_duration_seconds(uint16_t duration_sec);

        int64_t _pulse_gap_us = 60000;
        int64_t _pulse_step_us = 0;   
        uint64_t _next_pulse_time;        
        bool _reset_after_climb = true;
        bool _reset = false;
};
