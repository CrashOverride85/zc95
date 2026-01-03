_pulse_width_us = 150

Mode = {BURST = 1, CONSTANT = 2}
MenuId = {BURST_FREQ = 1, FREQ = 2, PULSE_WIDTH = 3, MODE = 4, PULSE_TYPE = 5}
PulseType = {BI = 1, MONO = 2}

_mode = Mode.BURST

_pulse_type  = PulseType.BI

_burst_freq_dhz = 30
_burst_duration_ms = 80
_burst_next_burst_ms = 0

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
            min = 1,
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
                {choice_id = Mode.BURST   , description = "Burst"},
                {choice_id = Mode.CONSTANT, description = "Constant"},
                {choice_id = 3, description = "Pusle rate mod"},
                {choice_id = 4, description = "Pusle wid mod 40%"},
                {choice_id = 5, description = "Pusle wid mod 70%"}
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
        SetFreq(_freq_hz)

    elseif (menu_id == MenuId.PULSE_WIDTH)
    then
        _pulse_width_us = min_max_val
        SetWidth()
    end
end

function MultiChoiceChange(menu_id, choice_id)
    if (menu_id == MenuId.MODE)
    then
        _mode = choice_id

        if (choice_id == Mode.BURST)
        then
            BurstStart()
        end

    elseif (menu_id == MenuId.PULSE_TYPE)
    then
        _pulse_type = choice_id
        SetWidth()
    end
end

function Loop(time_ms)

    if (_mode == Mode.BURST)
    then
        BurstLoop(time_ms);
    end
end

function BurstStart()
    _burst_next_burst_ms = 0
    zc.ChannelOff(1)
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
        _burst_next_burst_ms = _burst_next_burst_ms + ((1 / _burst_freq_dhz) * 10000)
    end
end

function SetFreq(freq_hz)
    zc.SetFrequency(1, freq_hz)
    zc.SetFrequency(2, freq_hz)
    zc.SetFrequency(3, freq_hz)
    zc.SetFrequency(4, freq_hz)
end

function SetWidth()
    if (_pulse_type == PulseType.MONO)
    then
        neg_wid = 1
    else
        neg_wid = _pulse_width_us
    end

    zc.SetPulseWidth(1, _pulse_width_us, neg_wid)
    zc.SetPulseWidth(2, _pulse_width_us, neg_wid)
    zc.SetPulseWidth(3, _pulse_width_us, neg_wid)
    zc.SetPulseWidth(4, _pulse_width_us, neg_wid)
end

