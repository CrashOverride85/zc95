
_progress_percent = 0
_duration_ms = 5000

Config = {
    name = "TriFade",
    allow_triphase = true,
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Cycle duration",
            id = 1,
            min = 100,
            max = 10000,
            increment_step = 100,
            uom = "ms",
            default = _duration_ms
         }
    }
}

function Setup(time_ms)
    zc.ChannelOn(1)

    zc.EnableTriphase(true)
    zc.LinkChannels(1, 2, 100)

    zc.SetPower(1, 1000)
    zc.SetPower(2, 1000)
    _step_start_time_ms = time_ms;
end

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1) then
        _duration_ms = min_max_val
    end
end

function OffsetCycle()
    local offset = 0
    if _progress_percent < 50 then
        offset = 100 - (_progress_percent*2)
    else
        offset = ((_progress_percent-50)*2)
    end

    zc.LinkChannels(1, 2, offset)
end

function UpdateStepProgres(time_ms)
    _progress_percent = ((time_ms - _step_start_time_ms) / _duration_ms) * 100

    if _progress_percent > 100 then
        _progress_percent = 0
        _step_start_time_ms = time_ms
    end
end

function Loop(time_ms)
    OffsetCycle()
    UpdateStepProgres(time_ms)
end
