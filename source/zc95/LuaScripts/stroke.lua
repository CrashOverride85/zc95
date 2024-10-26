-- "Stroke" - kindly contributed by someone who perfers to remain anonymous

require("ettot")

Config = {
    name = "Stroke",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Speed",
            id = 1,
            min = 0,
            max = 32,
            increment_step = 1,
            uom = "",
            default = 16
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
    channels["block"] = ettot.block_stroke
    ettot.setupblock(channels)
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["intensity"]["rate"] = min_max_val
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
       print("tick", elapsed_ms, _loop_count)
       _loop_count = 0
    end
end
