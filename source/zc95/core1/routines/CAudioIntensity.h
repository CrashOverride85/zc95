#include "CRoutine.h"

class CAudioIntensity: public CRoutine
{
    public:
        CAudioIntensity(uint8_t param);
        ~CAudioIntensity();
        static CRoutine* create(uint8_t param) { return new CAudioIntensity(param); };
        void get_config(struct routine_conf *conf);
        static void config(struct routine_conf *conf);
        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void menu_selected(uint8_t menu_id);
        void soft_button_pushed (soft_button button, bool pushed);
        void audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void start();
        void loop(uint64_t time_us);
        void stop();

    private:
        bool _mono;
};
