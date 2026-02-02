#include "CPowerLevelRamp.h"

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

float CPowerLevelRamp::get_ramp_percent()
{
    return _ramp_in_progress ? _ramp_percent : 100;
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

    _ramp_percent = ramp_progress_us / us_per_pp;
    _ramp_percent += _saved_settings->get_extended_ramp_level(); // ramp starting %

    if (_ramp_percent > 100)
    {
        _ramp_percent = 100;
        _ramp_in_progress = false;
    }
}
