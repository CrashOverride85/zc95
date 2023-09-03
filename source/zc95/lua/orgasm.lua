-- Orgasm

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

-- Defaults for all the modes
function setupdefaults(n)
    n["intensity"]["value"] = 255
    n["intensity"]["min"] = 222
    n["intensity"]["max"] = 255
    n["intensity"]["rate"] = 1
    n["intensity"]["steps"] = 1    
    n["intensity"]["actionmin"] = 255       
    n["intensity"]["actionmax"] = 255
    n["intensity"]["select"] = 0
    n["intensity"]["timer"] = 0
    n["intensity"]["changed"] = true    

    n["freq"]["value"] = 22
    n["freq"]["min"] = 9
    n["freq"]["max"] = 100
    n["freq"]["rate"] = 1
    n["freq"]["steps"] = 1    
    n["freq"]["actionmin"] = 255       
    n["freq"]["actionmax"] = 255
    n["freq"]["select"] = 0
    n["freq"]["timer"] = 0
    n["freq"]["changed"] = true

    n["width"]["value"] = 130
    n["width"]["min"] = 50    
    n["width"]["max"] = 200
    n["width"]["rate"] = 1
    n["width"]["steps"] = 1    
    n["width"]["actionmin"] = 255   
    n["width"]["actionmax"] = 255
    n["width"]["select"] = 0
    n["width"]["timer"] = 0       
    n["width"]["changed"] = true    
end

-- because old lua
function bxor (a,b)
local r = 0
for i = 0, 31 do
local x = a / 2 + b / 2
if x ~= math.floor (x) then
r = r + 2^i
end
a = math.floor (a / 2)
b = math.floor (b / 2)
end
return r
end

block_climb, block_climb5, block_climb6, block_climb7, block_climb8, block_climb9, block_climb10 = 50, 5, 6, 7, 8, 9, 10
block_orgasm, block_orgasm2, block_orgasm3, block_orgasm4 = 24, 25, 26, 27
block_phasing = 20

function setupblock(channels)
    for chan = 1, 4, 1
    do
        channels[chan]["width"]["changed"] = true
        channels[chan]["freq"]["changed"] = true
        channels[chan]["intensity"]["changed"] = true        
    end
--    print("**** NEW BLOCK ",channels["block"])
    if (channels["block"] == block_climb) then
          channels[1]["freq"]["select"] = 1
          channels[1]["freq"]["rate"] = 50  -- MA knob 1 to 100
          channels[1]["freq"]["value"] = 255
          channels[1]["freq"]["max"] = 255
          channels[1]["freq"]["steps"] = 1
          channels[1]["freq"]["actionmin"] = block_climb6

          channels[2]["freq"]["rate"] = 50  -- MA knob 1 to 100
          channels[2]["freq"]["select"] = 1
          channels[2]["freq"]["value"] = 255
          channels[2]["freq"]["max"] = 255
          channels[2]["freq"]["steps"] = 1
          channels[2]["freq"]["actionmin"] = block_climb9

    elseif (channels["block"] == block_climb5) then
          channels[1]["freq"]["select"] = 1
          channels[1]["freq"]["value"] = 255
          channels[1]["freq"]["max"] = 255
          channels[1]["freq"]["steps"] = 1
          channels[1]["freq"]["actionmin"] = block_climb6    
    elseif (channels["block"] == block_climb6) then
          channels[1]["freq"]["value"] = 255
          channels[1]["freq"]["steps"] = 2
          channels[1]["freq"]["actionmin"] = block_climb7
    elseif (channels["block"] == block_climb7) then
          channels[1]["freq"]["value"] = 255
          channels[1]["freq"]["steps"] = 4
          channels[1]["freq"]["actionmin"] = block_climb5
    elseif (channels["block"] == block_climb8) then        
          channels[2]["freq"]["select"] = 1
          channels[2]["freq"]["value"] = 255
          channels[2]["freq"]["max"] = 255
          channels[2]["freq"]["steps"] = 1
          channels[2]["freq"]["actionmin"] = block_climb9
    elseif (channels["block"] == block_climb9) then
          channels[2]["freq"]["value"] = 255
          channels[2]["freq"]["steps"] = 2
          channels[2]["freq"]["actionmin"] = block_climb10
    elseif (channels["block"] == block_climb10) then
          channels[2]["freq"]["value"] = 255
          channels[2]["freq"]["steps"] = 5
          channels[2]["freq"]["actionmin"] = block_climb8

    elseif (channels["block"] == block_phasing) then
        channels[1]["width"]["select"] = 0
        channels[2]["width"]["select"] = 0        
        channels[1]["width"]["value"] = 126
        channels[2]["width"]["value"] = 121       
        channels[1]["freq"]["value"] = 25
        channels[2]["freq"]["value"] = 25
        channels[1]["intensity"]["select"] = 1
        channels[2]["intensity"]["select"] = 1        
        channels[1]["intensity"]["rate"] = 5
        channels[2]["intensity"]["rate"] = 5
        channels[1]["intensity"]["min"] = 231
        channels[2]["intensity"]["min"] = 231

    elseif (channels["block"] == block_orgasm) then
        channels[1]["freq"]["value"] = 135
        channels[2]["freq"]["value"] = channels[1]["freq"]["value"]
        channels[1]["width"]["min"] = 50
        channels[2]["width"]["min"] = channels[1]["width"]["min"]
        channels[1]["width"]["value"] = channels[1]["width"]["min"]
        channels[2]["width"]["value"] = channels[1]["width"]["min"]
        channels[1]["width"]["steps"] = 4
        channels[2]["width"]["steps"] = channels[1]["width"]["steps"]
        channels[1]["width"]["rate"] = 1
        channels[2]["width"]["rate"] = channels[1]["width"]["rate"]
        channels[1]["width"]["select"] = 1
        channels[2]["width"]["select"] = 0
        channels[1]["width"]["actionmax"] = block_orgasm2
    elseif (channels["block"] == block_orgasm2) then
        channels[1]["width"]["steps"] = -1
        channels[1]["width"]["actionmin"] = block_orgasm3
        channels[2]["width"]["select"] = 1
        channels[2]["width"]["actionmax"] = 255
        channels[1]["width"]["min"] = (channels[1]["width"]["min"] + 2)%256
        channels[1]["width"]["min"] = bxor(channels[1]["width"]["min"],2)
        channels[2]["width"]["min"] = (channels[2]["width"]["min"] + 2)%256
        channels[2]["width"]["min"] = bxor(channels[2]["width"]["min"],2)
    elseif (channels["block"] == block_orgasm3) then
        channels[1]["width"]["select"] = 0
        channels[2]["width"]["actionmin"] = block_orgasm4
    elseif (channels["block"] == block_orgasm4) then
        channels[1]["width"]["select"] = 1
        channels[2]["width"]["select"] = 0
        channels[1]["width"]["steps"] = 1
        channels[2]["width"]["steps"] = 1          
    end            
end

function Setup(time_ms)
    print("SETUP")

    channels = {}
    for chan = 1, 4, 1
    do
        channels[chan] = {}
        channels[chan]["width"] = {}
        channels[chan]["freq"] = {}
        channels[chan]["intensity"] = {}        
        setupdefaults(channels[chan])
        
        zc.ChannelOn(chan)
        zc.SetPower(chan, 1000)
    end
    channels["block"] = block_orgasm
    setupblock(channels)
end

function MinMaxChange(menu_id, min_max_val)
    for chan = 1, 4, 1
    do
        channels[chan]["freq"]["value"] = min_max_val
        channels[chan]["freq"]["changed"] = true
    end
end

function handleblock(b)
    local changed = false

    if b["select"] == 0 then
       return false
    end
    b["timer"] = b["timer"] +1
    if b["timer"] > b["rate"] then
         b["timer"] = 0
         b["value"] = b["value"] + b["steps"]
         if (b["steps"] > 0) then
             if b["value"] >= b["max"] then
               b["value"] = b["max"]
               if b["actionmax"] == 255 then
                 b["steps"] = -b["steps"]
               elseif b["actionmax"] <200 then
                 channels["block"] = b["actionmax"]
                 setupblock(channels)
               end
           end
         else
           if b["value"] <= b["min"] then
             b["value"] = b["min"]
             if b["actionmin"] == 255 then
                 b["steps"] = -b["steps"]             
             elseif b["actionmin"] <200 then
                 channels["block"] = b["actionmin"]
                 setupblock(channels)
             end             
         end
         end
         changed = true
         b["changed"] = true
    end
    return changed
end

-- Orgasm mode only does channel A and B so make C and D copy it

function at244hz()
    for chan = 1, 2, 1
    do
        handleblock(channels[chan]["freq"])
        if channels[chan]["freq"]["changed"] then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
           zc.SetFrequency(chan+2, 3750/channels[chan]["freq"]["value"])           
           channels[chan]["freq"]["changed"] = false           
        end

        handleblock(channels[chan]["width"])
        if channels[chan]["width"]["changed"] then        
            zc.SetPulseWidth(chan, channels[chan]["width"]["value"], channels[chan]["width"]["value"])
            zc.SetPulseWidth(chan+2, channels[chan]["width"]["value"], channels[chan]["width"]["value"])            
            channels[chan]["width"]["changed"] = false            
        end

        handleblock(channels[chan]["intensity"])        
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
