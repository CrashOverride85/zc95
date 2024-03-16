#include "CFrontPanel.h"
#include "CRotEnc.h"
#include "../CSavedSettings.h"
#include "../config.h"

class CFrontPanelV02 : public CFrontPanel
{
    public:
        CFrontPanelV02(CSavedSettings *saved_settings);
        void process(bool always_update);
        uint16_t get_channel_power_level(uint8_t channel);
        int8_t get_adjust_control_change();
        void interrupt (interrupt_t i);
        bool button_state(enum Button button);
        bool has_button_state_changed(enum Button button, bool *new_state);
        void set_button_in_use(enum Button button, bool in_use);

    private:
        enum adc_reg_t
        {
            CONVERSION = 0,
            CONFIG = 1
        };

        enum adc_input_t
        {
            AIN0 = 4,
            AIN1 = 5,
            AIN2 = 6,
            AIN3 = 7
        };

        enum led_reg_t
        {
            MODE1,
            MODE2,
            PWM0,
            PWM1,
            PWM2,
            PWM3,
            PWM4,
            PWM5,
            PWM6,
            PWM7,
            GRPPWM,
            GRPFREQ,
            LEDOUT0,
            LEDOUT1,
            SUBADR1,
            SUBADR2,
            SUBADR3,
            ALLCALLADR,
            IREF,
            EFLAG
        };

        enum port_exp_reg_t
        {
            INPUT_PORT,
            OUTPUT_PORT,
            POLARITY_INVERSION,
            CONFIGURATION
        };

        enum port_exp_pin_t
        {
            BUTTON_A,   // P0, labelled J1 on PCB
            BUTTON_B,
            BUTTON_C,
            BUTTON_D,   // P3, labelled J4 on PCB
            LED_RESET,  // P4, connected to reset pin of LED driver TLC59108. The only output, the rest are inputs
            ROT_BUTTON, // P5, button part of rotary encoder
            ROT_A,      // P6, rotary encoder
            ROT_B       // P7, rotary encoder  
        };


        void read_adc();
        void rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos);
        bool read_port_expander(uint8_t *value);
        void init_adc();
        void init_port_exp();
        uint16_t update_adc_input(adc_input_t input); // returns adc_config_reg
        void adc_select_next_input();

        void write_adc_register(adc_reg_t reg, uint16_t value);
        bool read_adc_register(adc_reg_t reg, uint16_t *value);

        void write_led_register(led_reg_t reg, uint8_t value);
        void update_button_led_states();
        void update_button_led_state(enum Button button, uint8_t old_state, uint8_t new_state, led_reg_t reg, bool always_update);

        CSavedSettings *_saved_settings;
        uint8_t _last_port_exp_read;
        int16_t _power_level[MAX_CHANNELS];
        CRotEnc _rot_encoder;
        int16_t _adjust_value;
        volatile bool _interrupt;

        uint32_t _interrupt_time;


        uint16_t _adc_config_reg = 0x8583; // Power on default from datasheet
        adc_input_t _adc_selected_input;
        
        uint64_t _last_state_change[MAX_BUTTON_IDX];
        uint8_t _button_states_at_last_check;
        uint8_t _buttons_in_use = 0;
};
