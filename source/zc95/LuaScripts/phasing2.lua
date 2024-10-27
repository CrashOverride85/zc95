-- "Phasing 2" - kindly contributed by someone who perfers to remain anonymous

-- Phase 2 on the ET boxes is quite a nice pulsing even if not used with tri-phase
-- so lets emulate that. Phase 2 has no MA knob adjustment, but it does use the
-- stored advanced parameters, so lets make it changable.
-- Called Phasing so no confusion with tri-phase

require("ettot")

Config = {
    name = "Phasing 2",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Pulse Speed",
            id = 1,
            min = 180,
            max = 240,
            increment_step = 1,
            uom = "",
            default = 215
         }
    }
}

function Setup(time_ms)
    channels = {}
    for chan = 1, 4, 1
    do
        channels[chan] = {}
        ettot.setupdefaults(channels[chan])
        zc.ChannelOn(chan)
        zc.SetPower(chan, 1000)
    end
    channels["block"] = ettot.block_phasing
    ettot.setupblock(channels)
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["intensity"]["min"] = min_max_val
        channels[chan]["intensity"]["changed"] = true
    end
end

_last_ms = 0
_loop_count = 0
_last_244 = 0

function Loop(time_ms)
-- Lets run a function at 244Hz. Assume Loop runs faster (it does, about 600 a second)
    local hz244 = math.floor(time_ms/(1000/244))
    if hz244 ~= _last_244 then
       _last_244 = hz244
       ettot.at244hz(4)
       _loop_count = _loop_count + 1
    end
end
