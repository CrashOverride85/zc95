_pulse_width_us = 150

Mode = {BURST = 1, CONSTANT = 2, PULSE_RATE_MOD = 3, PULSE_WID_MOD_40 = 4, PULSE_WID_MOD_70 = 5}
MenuId = {BURST_FREQ = 1, FREQ = 2, PULSE_WIDTH = 3, MODE = 4, PULSE_TYPE = 5}
PulseType = {BI = 1, MONO = 2}

_mode = Mode.BURST

_pulse_type  = PulseType.BI

_burst_freq_dhz = 30
_burst_duration_ms = 80 -- const
_burst_next_burst_ms = 0

_pulse_rate_mod_cycle_duration_ms = 10000 -- const
_pulse_rate_mod_last_freq_hz = 10

_pulse_width_mod_cycle_duration_ms = 10000 -- const
_pulse_width_mod_last_us = 80

_freq_hz = 150

Config = {
    name = "TENS",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Burst frequency",
            id = MenuId.BURST_FREQ,
            min = 1,
            max = 50,
            increment_step = 1,
            uom = "dHz", -- Deci-hertz
            default = _burst_freq_dhz
        },
        {
            type = "MIN_MAX",
            title = "Frequency",
            id = MenuId.FREQ,
            min = 20,
            max = 150,
            increment_step = 1,
            uom = "Hz",
            default = _freq_hz
        },
        {
            type = "MIN_MAX",
            title = "Pulse width",
            id = MenuId.PULSE_WIDTH,
            min = 10,
            max = 255,
            increment_step = 5,
            uom = "us",
            default = _pulse_width_us
         },
         {
            type = "MULTI_CHOICE",
            title = "Mode",
            id = MenuId.MODE,
            choices = {
                {choice_id = Mode.BURST           , description = "Burst"},
                {choice_id = Mode.CONSTANT        , description = "Constant"},
                {choice_id = Mode.PULSE_RATE_MOD  , description = "Pusle rate mod"},
                {choice_id = Mode.PULSE_WID_MOD_40, description = "Pusle wid mod 40%"},
                {choice_id = Mode.PULSE_WID_MOD_70, description = "Pusle wid mod 70%"}
            }
         },
         {
            type = "MULTI_CHOICE",
            title = "Pulse type",
            id = MenuId.PULSE_TYPE,
            choices = {
                {choice_id = PulseType.BI  , description = "Biphasic"},
                {choice_id = PulseType.MONO, description = "Monophasic"}
            }
         }
    }
}

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == MenuId.BURST_FREQ)
    then
        _burst_freq_dhz = min_max_val

    elseif (menu_id == MenuId.FREQ)
    then
        _freq_hz = min_max_val

        if (_mode ~= Mode.PULSE_RATE_MOD)
        then
            SetFreq(_freq_hz)
        end

    elseif (menu_id == MenuId.PULSE_WIDTH)
    then
        _pulse_width_us = min_max_val
        SetWidth(_pulse_width_us)
    end
end

function MultiChoiceChange(menu_id, choice_id)
    if (menu_id == MenuId.MODE)
    then
        _mode = choice_id

        if (choice_id == Mode.BURST)
        then
            _burst_next_burst_ms = 0
            PowerOff()
        else
            PowerOn()
        end

        SetWidth(_pulse_width_us)
        SetFreq(_freq_hz)

    elseif (menu_id == MenuId.PULSE_TYPE)
    then
        _pulse_type = choice_id

        if (_mode == Mode.PULSE_WID_MOD_40 or _mode == Mode.PULSE_WID_MOD_70)
        then
            SetWidth(_pulse_width_mod_last_us)
        else
            SetWidth(_pulse_width_us)
        end
    end
end

function Loop(time_ms)

    if (_mode == Mode.BURST)
    then
        BurstLoop(time_ms);

    elseif (_mode == Mode.PULSE_RATE_MOD)
    then
        PulRateModLoop(time_ms)

    elseif (_mode == Mode.PULSE_WID_MOD_40)
    then
        PulWidModLoop(time_ms, 40)

    elseif (_mode == Mode.PULSE_WID_MOD_70)
    then
        PulWidModLoop(time_ms, 70)
    end
end

function BurstLoop(time_ms)

    if (_burst_next_burst_ms == 0)
    then
        _burst_next_burst_ms = time_ms + ((1 / _burst_freq_dhz) * 10000)
        return
    end

    if (time_ms > _burst_next_burst_ms)
    then
        zc.ChannelPulseMs(1, _burst_duration_ms)
        zc.ChannelPulseMs(2, _burst_duration_ms)
        zc.ChannelPulseMs(3, _burst_duration_ms)
        zc.ChannelPulseMs(4, _burst_duration_ms)
        _burst_next_burst_ms = _burst_next_burst_ms + ((1 / _burst_freq_dhz) * 10000)
    end
end

function PulRateModLoop(time_ms)
    local lower_hz = _freq_hz * 0.6
    local vary_hz = _freq_hz - lower_hz
    local period = _pulse_rate_mod_cycle_duration_ms

    -- Generate triangle wave, going between the configured frequency (_freq_hz), and the configured frequency minus 40% (lower_hz)
    freq = ((vary_hz / math.pi) * math.asin( math.sin( (2 * math.pi / period ) * time_ms )  )) + (vary_hz / 2) + lower_hz

    local freq_floor = math.floor(freq)
    if (freq_floor ~= _pulse_rate_mod_last_freq_hz)
    then
        _pulse_rate_mod_last_freq_hz = freq_floor
        SetFreq(freq_floor)
    end
end

function PulWidModLoop(time_ms, vary_percent)
    local lower_us = _pulse_width_us * ((100 - vary_percent) / 100)
    local vary_us = _pulse_width_us - lower_us
    local period = _pulse_width_mod_cycle_duration_ms

    -- Generate triangle wave, going between the configured pulse width (_pulse_width_us), and the configured pulse width minus vary_percent (lower_us)
    freq = ((vary_us / math.pi) * math.asin( math.sin( (2 * math.pi / period ) * time_ms )  )) + (vary_us / 2) + lower_us

    local us_floor = math.floor(freq)
    if (us_floor ~= _pulse_width_mod_last_us)
    then
        _pulse_width_mod_last_us = us_floor
        SetWidth(us_floor)
    end
end


function SetFreq(freq_hz)
    zc.SetFrequency(1, freq_hz)
    zc.SetFrequency(2, freq_hz)
    zc.SetFrequency(3, freq_hz)
    zc.SetFrequency(4, freq_hz)
end

function SetWidth(pulse_width_us)
    if (_pulse_type == PulseType.MONO)
    then
        neg_wid = 1
    else
        neg_wid = pulse_width_us
    end

    zc.SetPulseWidth(1, pulse_width_us, neg_wid)
    zc.SetPulseWidth(2, pulse_width_us, neg_wid)
    zc.SetPulseWidth(3, pulse_width_us, neg_wid)
    zc.SetPulseWidth(4, pulse_width_us, neg_wid)
end

function PowerOn()
    zc.ChannelOn(1)
    zc.ChannelOn(2)
    zc.ChannelOn(3)
    zc.ChannelOn(4)
end

function PowerOff()
    zc.ChannelOff(1)
    zc.ChannelOff(2)
    zc.ChannelOff(3)
    zc.ChannelOff(4)
end
