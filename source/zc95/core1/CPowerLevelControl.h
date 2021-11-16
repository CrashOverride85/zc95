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

        // Power level being requested by routine (0-1000)
        void set_routine_requested_power_level(uint8_t channel, uint16_t power);

        // Get power level to send to output chanel (0-1000)
        // (this is the yellow bar inside the bar graph)
        uint16_t get_output_power_level(uint8_t channel);
        
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
        uint16_t _routine_power[MAX_CHANNELS];
        uint16_t _output_power[MAX_CHANNELS];

        const uint8_t  _ramp_duration_secs = 5;
        uint8_t  _ramp_percent = 0; // 100=full power
        uint64_t _ramp_last_increment_us = 0;
        uint16_t _ramp_increment_period_ms = 0;
        bool _ramp_in_progress = false;

        CSavedSettings *_saved_settings;
};

#endif
