#include "CRoutine.h"

class CTens: public CRoutine
{
    public:
        CTens(uint8_t param);
        ~CTens();
        static CRoutine* create(uint8_t param) { return new CTens(param); }
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void start();
        void loop(uint64_t time_us);
        void stop();

        static const uint8_t InitalFrequency = 50;

    private:
        void set_pulse_width_all(uint8_t pulse_width);
        void set_freq_all(uint16_t freq);
};
