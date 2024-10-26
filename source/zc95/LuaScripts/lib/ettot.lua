-- Library to handle Waves, Orgasm & Climb - code kindly contributed by someone who perfers to remain anonymous

module("ettot", package.seeall)

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
    n["intensity"]["gateoff"] = false
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

    n["gate"] = {}
    n["gate"]["select"] = 0
    n["gate"]["timer"] = 0
    n["gate"]["rate"] = 0
    n["gate"]["gateoff"] = false
    n["gate"]["changed"] = true

    n["blocktimer"] = {}
    n["blocktimer"]["select"] = 0
    n["blocktimer"]["timer"] = 0
    n["blocktimer"]["rate"] = 0
    n["blocktimer"]["nextblock"] = 0    
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
block_waves = 11
block_stroke = 3
block_rhythm, block_rhythm16, block_rhythm17 = 15, 16, 17

function setupblock(channels)
    print("setupblock");
    for chan = 1, 4, 1
    do
        channels[chan]["width"]["changed"] = true
        channels[chan]["freq"]["changed"] = true
        channels[chan]["intensity"]["changed"] = true        
    end
    print("**** NEW BLOCK ",channels["block"])
    if (channels["block"] == block_waves) then
          channels[1]["freq"]["max"] = 128
          channels[3]["freq"]["max"] = channels[1]["freq"]["max"]
          channels[2]["freq"]["max"] = 64
          channels[4]["freq"]["max"] = channels[2]["freq"]["max"]
          channels[1]["width"]["steps"] = 2
          channels[2]["width"]["steps"] = 3
          channels[3]["width"]["steps"] = channels[2]["width"]["steps"]
          channels[4]["width"]["steps"] = channels[1]["width"]["steps"]
          for chan = 1, 4, 1
          do
              channels[chan]["freq"]["select"] = 1
              channels[chan]["freq"]["rate"] = 32
              channels[chan]["width"]["select"] = 1
              channels[chan]["width"]["rate"] = channels[chan]["freq"]["rate"]
          end
    elseif (channels["block"] == block_stroke) then
          channels[1]["intensity"]["gateoff"] = true
          channels[1]["intensity"]["steps"] = 2
          channels[1]["intensity"]["rate"] = 16 -- MA knob 0 to 32
          channels[1]["intensity"]["select"] = 1
          -- intensity minimum is range_depth_high (255) - depth_advanced (215) + range_depth_low (165) = 205          
          channels[1]["intensity"]["min"] = 205
          channels[1]["intensity"]["actionmin"] = 254 -- xor gate with 110 then same as 255
          channels[1]["intensity"]["actionmax"] = 254 -- xor gate with 110 then same as 255
          channels[1]["width"]["value"] = 255
          channels[2]["intensity"]["select"] = 1
          channels[2]["intensity"]["rate"] = 16 -- MA knob 0 to 32
          channels[2]["intensity"]["min"] = 230
          channels[2]["width"]["value"] = 216

          channels[3]["intensity"]["gateoff"] = true
          channels[3]["intensity"]["steps"] = 2
          channels[3]["intensity"]["rate"] = 16 -- MA knob 0 to 32
          channels[3]["intensity"]["select"] = 1
          channels[3]["intensity"]["min"] = 205
          channels[3]["intensity"]["value"] = channels[3]["intensity"]["min"]          -- start it offset
          channels[3]["intensity"]["actionmin"] = 254 -- xor gate with 110 then same as 255
          channels[3]["intensity"]["actionmax"] = 254 -- xor gate with 110 then same as 255
          channels[3]["width"]["value"] = 255
          channels[4]["intensity"]["select"] = 1
          channels[4]["intensity"]["rate"] = 16 -- MA knob 0 to 32
          channels[4]["intensity"]["min"] = 230
          channels[3]["intensity"]["value"] = channels[4]["intensity"]["min"]          -- start it offset
          channels[4]["width"]["value"] = 216
    elseif (channels["block"] == block_rhythm) then
          for chan = 1, 4, 1
          do    
            channels[chan]["width"]["value"] = 70
            channels[chan]["intensity"]["value"] = 224
            channels[chan]["intensity"]["min"] = 224      
            channels[chan]["intensity"]["actionmin"] = 253 -- 0xfd goto max
            channels[chan]["intensity"]["actionmax"] = 253 -- 0xfd goto min
            channels[chan]["intensity"]["select"] = 1
            channels[chan]["intensity"]["steps"] = 0
            channels[chan]["gate"]["select"] = 1
            channels[chan]["gate"]["rate"] = 12
          end
          channels[1]["blocktimer"]["select"] = 1
          channels[1]["blocktimer"]["rate"] = 31
          channels[1]["blocktimer"]["nextblock"] = 16
          
    elseif (channels["block"] == block_rhythm16) then
          channels[1]["blocktimer"]["nextblock"] = 17
          for chan = 1, 4, 1
          do    
            channels[chan]["width"]["value"] = 196
            channels[chan]["intensity"]["value"] = channels[chan]["intensity"]["value"] + 1
          end
    elseif (channels["block"] == block_rhythm17) then
          channels[1]["blocktimer"]["nextblock"] = 16    
          for chan = 1, 4, 1
          do    
            channels[chan]["width"]["value"] = 70
          end
    elseif (channels["block"] == block_climb) then
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
          for chan = 1, 4, 1
          do
                channels[chan]["width"]["value"] = 129 - 4*chan
                channels[chan]["freq"]["value"] = 25
                channels[chan]["intensity"]["select"] = 1
                channels[chan]["intensity"]["min"] = 215
          end
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

function handleblocktimer(b)
    if b["select"] == 0 then
      return
    end
    b["timer"] = b["timer"] +1
    if b["timer"] > (8*b["rate"]) then
      b["timer"] = 0
      channels["block"] = b["nextblock"]
      setupblock(channels)
    end
end
    
function handlegatetimer(b)
    if b["select"] == 0 then
      return
    end
    b["timer"] = b["timer"] +1
    if b["timer"] > b["rate"] then
      b["timer"] = 0
      b["gateoff"] = not b["gateoff"]
      b["changed"] = true
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
         if b["value"] >= b["max"] then
             b["value"] = b["max"]
             if b["actionmax"] == 255 then
                 b["steps"] = -b["steps"]
             elseif b["actionmax"] == 254 then
                 b["steps"] = -b["steps"]
                 b["gateoff"] = not b["gateoff"]
             elseif b["actionmin"] == 253 then
                 b["value"] = b["min"]
             elseif b["actionmax"] <200 then
                 channels["block"] = b["actionmax"]
                 setupblock(channels)
             end
         elseif b["value"] <= b["min"] then
             b["value"] = b["min"]
             if b["actionmin"] == 255 then
                 b["steps"] = -b["steps"]
             elseif b["actionmin"] == 254 then
                 b["steps"] = -b["steps"]
                 b["gateoff"] = not b["gateoff"]
             elseif b["actionmin"] == 253 then
                 b["value"] = b["max"]
             elseif b["actionmin"] <200 then
                 channels["block"] = b["actionmin"]
                 setupblock(channels)
             end             
         end
         changed = true
         b["changed"] = true
    end
    return changed
end

function at244hz()
    for chan = 1, 4, 1
    do
        ettot.handleblocktimer(channels[chan]["blocktimer"])
        ettot.handlegatetimer(channels[chan]["gate"])
        
        ettot.handleblock(channels[chan]["freq"])
        if channels[chan]["freq"]["changed"] then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
           channels[chan]["freq"]["changed"] = false
        end

        ettot.handleblock(channels[chan]["width"])
        if channels[chan]["width"]["changed"] then
            zc.SetPulseWidth(chan, channels[chan]["width"]["value"], channels[chan]["width"]["value"])
            channels[chan]["width"]["changed"] = false
        end

        ettot.handleblock(channels[chan]["intensity"])
        if (channels[chan]["intensity"]["changed"] or channels[chan]["gate"]["changed"]) then
            if (channels[chan]["intensity"]["gateoff"] or channels[chan]["gate"]["gateoff"]) then
               zc.SetPower(chan, 0)
            else
               zc.SetPower(chan, channels[chan]["intensity"]["value"]*1000/255)
            end
            channels[chan]["intensity"]["changed"] = false
            channels[chan]["gate"]["changed"] = false            
        end
    end
end
