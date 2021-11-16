#include "CRoutineOutput.h"
#include "Core1.h"
#include "../display/CDisplay.h"

class CRoutineOutputDebug : public CRoutineOutput
{
    public:
        CRoutineOutputDebug(Core1 *core1, CDisplay *display);
        void set_front_panel_power(uint8_t channel, uint16_t power);
        uint16_t get_output_power(uint8_t channel);
        uint16_t get_front_pannel_power(uint8_t channel);
        
        void activate_routine(uint8_t routine_id);
        void stop_routine();

        void menu_min_max_change(uint8_t menu_id, int16_t new_value);
        void menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id);
        void trigger(trigger_socket socket, trigger_part part, bool active);
        void soft_button_pressed(soft_button button);
        void loop();
        void reinit_channels();
        void collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power);

    private:
        void update_display(uint8_t channel);

        Core1 *_core1 = NULL;
        CDisplay *_display;       
};


