#include "CRoutine.h"

class CCamTrigger: public CRoutine
{
    public:
        CCamTrigger();
        ~CCamTrigger();
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void soft_button_pushed (soft_button button, bool pushed);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        void all_channels(bool on);
        void all_channels_pulse(uint16_t pulse_len_ms);
        int16_t _pulse_len_ms;
        uint16_t _cam_delay_ms;
        uint64_t _cam_pulse_start_us;
        uint64_t _cam_pulse_end_us;

        uint64_t _shock_pulse_start_us = 0;

        
};
