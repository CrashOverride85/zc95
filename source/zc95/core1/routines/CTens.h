#include "CRoutine.h"

class CTens: public CRoutine
{
    public:
        CTens();
        ~CTens();
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void start();
        void loop(uint64_t time_us);
        void stop();

        static const uint8_t InitalFrequency = 50;

    private:
        uint64_t hz_to_us_delay(int16_t hz);
     
        uint16_t _pulse_width_us;
        uint16_t  _freq_hz;

        int16_t _speed_setting;
        uint64_t _next_pulse_time;
        uint8_t _current_active_channel;       
};
