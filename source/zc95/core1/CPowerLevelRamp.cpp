#include "CPowerLevelRamp.h"
#include <math.h>

CPowerLevelRamp::CPowerLevelRamp(CSavedSettings *saved_settings)
{
    _saved_settings = saved_settings;
    reset();
}

void CPowerLevelRamp::reset()
{
    _last_calc_us = 0;
    _ramp_in_progress = false;
}

bool CPowerLevelRamp::loop()
{
    if (time_us_64() - _last_calc_us > (100 * 1000)) // calc every 100ms
    {
        calc_ramp_percent();
        _last_calc_us = time_us_64();
        return true;
    }

    return false;
}

float CPowerLevelRamp::get_ramp_power_percent()
{
    if (!_ramp_in_progress)
        return 100;

    float shape = (float)_saved_settings->get_extended_ramp_shape() / (float)10;
    float shape_adjusted_ramp = s_normalized_exponential(shape, _ramp_percent / (float)100);
    return shape_adjusted_ramp * 100;
}

bool CPowerLevelRamp::ramp_in_progress()
{
    return _ramp_in_progress;
}

void CPowerLevelRamp::ramp_start()
{
    printf("Extended ramp started\n");
    _ramp_start_time_us = time_us_64();
    _ramp_in_progress = true;
    _ramp_percent = 0;
    calc_ramp_percent();
}

void CPowerLevelRamp::calc_ramp_percent()
{
    if (!_ramp_in_progress)
        return;

    uint32_t us_per_pp = (uint32_t)_saved_settings->get_extended_ramp_time_seconds() * (uint32_t)1000000;
    uint32_t ramp_progress_us = time_us_64() - _ramp_start_time_us;

    _ramp_percent = (float)ramp_progress_us / (float)us_per_pp;
    _ramp_percent += _saved_settings->get_extended_ramp_level(); // ramp starting %

    if (_ramp_percent > 100)
    {
        _ramp_percent = 100;
        _ramp_in_progress = false;
    }
}

// k = ramp shape: -ve values will cause a fast initial rise, then slow down. +ve values will cause a slow inital rise, then speed up.
//                  0 will result in a purley linear rise (return value = t)
// t = time, 0 to 1, i.e. 0.5 = 50% through the ramp time-wise 
// Output is 0 to 1, which should be mapped onto an output level of 0 to 1000.
float CPowerLevelRamp::s_normalized_exponential(float k, float t)
{
    // As k -> 0, the function approaches t
    if (fabsf(k) < 1e-9)
    {
        return t;
    }

    return (exp(k * t) - 1.0) /
           (exp(k) - 1.0);
}
