#include "CRoutine.h"

class CPredicament: public CRoutine
{
    public:
        CPredicament();
        ~CPredicament();
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
        bool _output_on = false;
        bool _trigger1_active;
        bool _trigger2_active;

        bool _trigger1_invert;
        bool _trigger2_invert;
        bool _logic_and;
        bool _output_invert;

        void all_channels(bool on);
        bool get_required_output_state();
};
