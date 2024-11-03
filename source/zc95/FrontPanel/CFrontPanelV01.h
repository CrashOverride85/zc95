#include "CFrontPanel.h"
#include "CRotEnc.h"
#include "CMainBoardPortExp.h"
#include "../CSavedSettings.h"
#include "../config.h"

class CFrontPanelV01 : public CFrontPanel
{
    public:
        CFrontPanelV01(CSavedSettings *saved_settings, CMainBoardPortExp *main_board_port_exp);
        void process(bool always_update);
        uint16_t get_channel_power_level(uint8_t channel);
        int8_t get_adjust_control_change();
        void interrupt (interrupt_t i);
        bool button_state(enum Button button);
        bool has_button_state_changed(enum Button button, bool *new_state);

    private:
        void read_adc();
        void rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos);
        uint8_t read_port_expander();
        CMainBoardPortExp *_main_board_port_exp;
        uint8_t _last_port_exp_read;
        int16_t _power_level[MAX_CHANNELS];
        CRotEnc _rot_encoder;
        bool _last_rot_button_state;
        uint64_t _last_rot_button_state_change;
        int16_t _adjust_value;
        volatile bool _interrupt;

        uint32_t _interrupt_time;
};
