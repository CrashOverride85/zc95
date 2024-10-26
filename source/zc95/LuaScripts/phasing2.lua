-- "Phasing 2" - kindly contributed by someone who perfers to remain anonymous

-- Phase 2 on the ET boxes is quite a nice pulsing even if not used with tri-phase
-- so let us emulate that. Phase 2 has no MA knob adjustment, but it does use the
-- stored advanced parameters, so we can make say the min intensity variable which
-- alters the speed a little. Call it Phasing so no confusion with tri-phase ET
-- mode

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
    print("SETUP")

    channels = {}
    for chan = 1, 4, 1
    do
        channels[chan] = {}
        channels[chan]["width"] = {}
        channels[chan]["freq"] = {}
        channels[chan]["intensity"] = {}
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

-- Lets run a function at 244Hz
-- Since we found Loop runs about 600 times a second we can just assume it's more than 244 times a second and this should work fine

    local hz244 = math.floor(time_ms/(1000/244))
    if hz244 ~= _last_244 then
       _last_244 = hz244
       ettot.at244hz()
       _loop_count = _loop_count + 1
    end

-- Just a debug tick to make sure everything was working

    local elapsed_ms = time_ms - _last_ms
    if elapsed_ms >= 1000 then
       _last_ms = time_ms
       print("******* tick", elapsed_ms, _loop_count, channels["block"], channels[1]["intensity"]["value"], channels[1]["width"]["value"], channels[1]["freq"]["value"])
       _loop_count = 0
    end
end
