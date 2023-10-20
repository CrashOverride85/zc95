-- "Orgasm" - kindly contributed by someone who perfers to remain anonymous

require("ettot")
block_orgasm, block_orgasm2, block_orgasm3, block_orgasm4 = 24, 25, 26, 27

Config = {
    name = "Orgasm",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Frequency",
            id = 1,
            min = 15,
            max = 255,
            increment_step = 8,
            uom = "",
            default = 135
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
    channels["block"] = block_orgasm
    ettot.setupblock(channels)
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["freq"]["value"] = min_max_val
        channels[chan]["freq"]["changed"] = true
    end
end

-- Orgasm mode only does channel A and B so make C and D copy it

function at244hz()
    for chan = 1, 2, 1
    do
        ettot.handleblock(channels[chan]["freq"])
        if channels[chan]["freq"]["changed"] then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
           zc.SetFrequency(chan+2, 3750/channels[chan]["freq"]["value"])           
           channels[chan]["freq"]["changed"] = false           
        end

        ettot.handleblock(channels[chan]["width"])
        if channels[chan]["width"]["changed"] then        
            zc.SetPulseWidth(chan, channels[chan]["width"]["value"], channels[chan]["width"]["value"])
            zc.SetPulseWidth(chan+2, channels[chan]["width"]["value"], channels[chan]["width"]["value"])            
            channels[chan]["width"]["changed"] = false            
        end

        ettot.handleblock(channels[chan]["intensity"])        
        if channels[chan]["intensity"]["changed"] then
            zc.SetPower(chan, channels[chan]["intensity"]["value"]*1000/255)
            zc.SetPower(chan+2, channels[chan]["intensity"]["value"]*1000/255)            
            channels[chan]["width"]["intensity"] = false            
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
--       print("******* tick", elapsed_ms, _loop_count, channels["block"], channels[1]["width"]["min"])
       _loop_count = 0
    end
end
