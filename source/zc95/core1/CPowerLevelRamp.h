#ifndef _CPOWERLEVELRAMP_H
#define _CPOWERLEVELRAMP_H

#include <inttypes.h>
#include "../CSavedSettings.h"
#include "../../common/zc95_config.h"

class CPowerLevelRamp
{
    public:
        CPowerLevelRamp(CSavedSettings *saved_settings);
        void ramp_start();
        bool loop();
        float get_ramp_power_percent();
        float get_ramp_progress_percent();
        bool ramp_in_progress();
        void reset();

        // used for ramp shape - both during ramp, and on the ramp config screen
        static float s_normalized_exponential(float k, float t);

    private:
        void calc_ramp_percent();

        float  _ramp_percent = 0; // 100=full power
        bool _ramp_in_progress = false;
        uint64_t _ramp_start_time_us = 0;
        uint64_t _last_calc_us = 0;
        CSavedSettings *_saved_settings;
};

#endif
