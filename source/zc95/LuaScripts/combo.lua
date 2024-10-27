-- "Combo" - kindly contributed by someone who perfers to remain anonymous

require("ettot")

Config = {
    name = "Combo",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Speed",
            id = 1,
            min = 1,
            max = 64,
            increment_step = 2,
            uom = "",
            default = 32
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
    channels["block"] = ettot.block_combo
    ettot.setupblock(channels)
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["gate"]["rate"] = min_max_val*8
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
