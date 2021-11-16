#include "CRoutine.h"

class CToggle: public CRoutine
{
    public:
        CToggle();
        ~CToggle();
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        uint64_t mHz_to_us_delay(int16_t mHz); // millihertz 

        int16_t _speed_setting;
        uint64_t _wait_until_us;
        uint8_t _current_active_channel;
        bool _pulse_mode = false;
        const int CHANNEL_COUNT = 4;
};
