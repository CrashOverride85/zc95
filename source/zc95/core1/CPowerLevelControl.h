#ifndef _CPOWERLEVELCONTROL_H
#define _CPOWERLEVELCONTROL_H

#include <inttypes.h>
#include "../CSavedSettings.h"
#include "../config.h"

class CPowerLevelControl
{
    public:
        CPowerLevelControl(CSavedSettings *saved_settings);

        // Call with the power level set on the front panel 
        // power is 0-1000, channel is 0-3
        void set_front_panel_power(uint8_t channel, uint16_t power);

        // Call with the power level set remotely
        // power is 0-1000, channel is 0-3
        void set_remote_power(uint8_t channel, uint16_t power);

        // Enable remote mode; front panel power acts as power limit, and remote power as the user selected power level
        void remote_mode_enable();

        // Disable remote modes; restore normal/default operation - remote power setting is ignored
        void remote_mode_disable();

        // Power level being requested by routine (0-1000)
        void set_routine_requested_power_level(uint8_t channel, uint16_t power);

        // Get power level to send to output chanel. Range varies based on set power level:
        //  - High   = 0 - 1000
        //  - Medium = 0 -  666
        //  - Low    = 0 -  333
        uint16_t get_output_power_level(uint8_t channel);

        // Get power level to display in the yellow graph. 
        // When in High power mode (default), this will always return the same as get_output_power_level()
        uint16_t get_display_power_level(uint8_t channel);
        
        // Get the current maximum power level (0-1000)
        // After ramp up, this will return the same as get_target_max_power_level.
        // (this is the main blue bar on the graph, minus the floating bit at the top during ramp up)
        uint16_t get_max_power_level(uint8_t channel);

        // Get the maximum power level (power level set on front pannel - 0-1000)
        // (this is the top of blue bar on the graph, which during ramp up, will be floating)
        uint16_t get_target_max_power_level(uint8_t channel);
        
        void ramp_start();

        void zero_power_level();

        void loop();

    private:
        void calc_output_power(uint8_t channel);
        uint16_t _front_panel_power[MAX_CHANNELS];
        uint16_t _remote_access_power[MAX_CHANNELS];
        uint16_t _routine_power[MAX_CHANNELS];
        uint16_t _output_power[MAX_CHANNELS];

        uint8_t  _ramp_percent = 0; // 100=full power
        uint64_t _ramp_last_increment_us = 0;
        uint16_t _ramp_increment_period_ms = 0;
        bool _ramp_in_progress = false;
        bool _remote_mode_active = false;

        CSavedSettings *_saved_settings;
};

#endif
