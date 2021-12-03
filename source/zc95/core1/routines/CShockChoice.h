#include "CRoutine.h"

class CShockChoice: public CRoutine
{
    public:
        CShockChoice();
        ~CShockChoice();
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
        enum channel
        {
            CHAN_A,   // channel 1
            CHAN_B    // channel 2 + 3
        };

        void reset();
        void reset_timer();
        int64_t secs_to_us(int seconds);
        int64_t time_remaining_us();
        void pulse_chan_and_inc_power(channel chan, uint16_t duration);

        uint64_t _timer_started_us;
        int16_t _choice_freq_sec;
        bool _warning_issued;
        int16_t _shock_inc_by = 0;
        uint16_t _chanA_power;
        uint16_t _chanB_power;
};
