-- WAVES

Config = {
    name = "Waves",
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
    print("SETUP")

    channels = {}
    for chan = 1, 4, 1
    do
        channels[chan] = {}
        channels[chan]["freq"] = {}
        channels[chan]["freq"]["timer"] = 0
        channels[chan]["freq"]["min"] = 9
        if chan == 1 or chan == 3 then
            channels[chan]["freq"]["max"] = 128
        else
            channels[chan]["freq"]["max"] = 64
        end
        channels[chan]["freq"]["value"] = channels[chan]["freq"]["min"]
        channels[chan]["freq"]["rate"] = 32
        channels[chan]["freq"]["steps"] = 1

        channels[chan]["us"] = {}
        channels[chan]["us"]["timer"] = 0
        channels[chan]["us"]["min"] = 50
        channels[chan]["us"]["max"] = 200
        channels[chan]["us"]["value"] = channels[chan]["us"]["min"]
        channels[chan]["us"]["rate"] = 32
        if chan == 1 or chan == 4 then
           channels[chan]["us"]["steps"] = 2
        else
           channels[chan]["us"]["steps"] = 3
        end

        zc.ChannelOn(chan)
        zc.SetPower(chan, 1000)
    end
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["freq"]["rate"] = min_max_val
        channels[chan]["us"]["rate"] = min_max_val
    end
end

function handleblock(b)
    local changed = false

    b["timer"] = b["timer"] +1
    if b["timer"] > b["rate"] then
         b["timer"] = 0
         b["value"] = b["value"] + b["steps"]
         if b["value"] >= b["max"] then
             b["value"] = b["max"]
             b["steps"] = -b["steps"]
         end
         if b["value"] <= b["min"] then
             b["value"] = b["min"]
             b["steps"] = -b["steps"]
         end
         changed = true
    end
    return changed
end

function at244hz()
    for chan = 1, 4, 1
    do
        if handleblock(channels[chan]["freq"]) then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
        end
        if handleblock(channels[chan]["us"]) then
            zc.SetPulseWidth(chan, channels[chan]["us"]["value"], channels[chan]["us"]["value"])
        end
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
       at244hz()
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
