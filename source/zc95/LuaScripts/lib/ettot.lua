-- Library to handle ET modes - code kindly contributed by someone who perfers to remain anonymous

module("ettot", package.seeall)

-- Defaults for all the modes
function setupdefaults(n)
    n["intensity"] = {}
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

    n["freq"] = {}
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

    n["width"] = {}
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

block_stroke = 3
block_climb, block_climb5, block_climb6, block_climb7, block_climb8, block_climb9, block_climb10 = 50, 5, 6, 7, 8, 9, 10
block_waves = 11
block_combo = 13
block_rhythm, block_rhythm16, block_rhythm17 = 15, 16, 17
block_phasing = 20
block_orgasm, block_orgasm2, block_orgasm3, block_orgasm4 = 24, 25, 26, 27
block_torment, block_torment1, block_torment2, block_torment3 = 28, 29, 30, 31
block_random2 = 32

function setupblock(channels)
    for chan = 1, 4, 1
    do
        channels[chan]["width"]["changed"] = true
        channels[chan]["freq"]["changed"] = true
        channels[chan]["intensity"]["changed"] = true
        channels[chan]["gate"]["changed"] = true
    end
    print("* NEW BLOCK=",channels["block"])
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
              channels[chan]["freq"]["rate"] = 32 -- MA knob
              channels[chan]["width"]["select"] = 1
              channels[chan]["width"]["rate"] = channels[chan]["freq"]["rate"]
          end
    elseif (channels["block"] == block_stroke) then
          for chan = 1, 4, 1
          do
             channels[chan]["intensity"]["steps"] = 2
             channels[chan]["intensity"]["select"] = 1
             -- intensity minimum is range_depth_high (255) - depth_advanced (215) + range_depth_low (165) = 205
             channels[chan]["intensity"]["min"] = 205
             channels[chan]["intensity"]["rate"] = 16 -- MA knob 0 to 32
             channels[chan]["intensity"]["actionmin"] = 254 -- xor gate with 110 then same as 255
             channels[chan]["intensity"]["actionmax"] = 254 -- xor gate with 110 then same as 255
             channels[chan]["intensity"]["gateoff"] = true
             channels[chan]["width"]["value"] = 255
          end

          for chan = 2, 4, 2
          do
              channels[chan]["intensity"]["actionmin"] = 255
              channels[chan]["intensity"]["actionmax"] = 255
              channels[chan]["intensity"]["gateoff"] = false
              channels[chan]["intensity"]["select"] = 1
              channels[chan]["intensity"]["rate"] = 16 -- MA knob 0 to 32
              channels[chan]["intensity"]["min"] = 230
              channels[chan]["intensity"]["steps"] = 1
              channels[chan]["width"]["value"] = 216
          end

          channels[3]["intensity"]["value"] = channels[3]["intensity"]["min"] -- start it offset
          channels[4]["intensity"]["value"] = channels[4]["intensity"]["min"] -- start it offset

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
          channels[1]["blocktimer"]["nextblock"] = block_rhythm16
    elseif (channels["block"] == block_rhythm16) then
          channels[1]["blocktimer"]["nextblock"] = block_rhythm17
          for chan = 1, 4, 1
          do
            channels[chan]["width"]["value"] = 196
            channels[chan]["intensity"]["value"] = (channels[chan]["intensity"]["value"] + 1)%256
          end
    elseif (channels["block"] == block_rhythm17) then
          channels[1]["blocktimer"]["nextblock"] = block_rhythm16
          for chan = 1, 4, 1
          do
            channels[chan]["width"]["value"] = 70
          end
    elseif (channels["block"] == block_combo) then
          channels[2]["freq"]["steps"] = 2
          channels[2]["width"]["steps"] = 2
          channels[4]["freq"]["steps"] = 2
          channels[4]["width"]["steps"] = 2
          for chan = 1, 4, 1
          do
              channels[chan]["gate"]["select"] = 1
              channels[chan]["gate"]["rate"] = 256
              channels[chan]["freq"]["select"] = 1
              channels[chan]["freq"]["rate"] = 8 -- default *8
              channels[chan]["width"]["select"] = 1
              channels[chan]["width"]["rate"] = 40 -- advanced param 5 *8
              channels[chan]["width"]["value"] = 130 -- advanced param 130
          end
    elseif (channels["block"] == block_torment) then
        for chan = 1, 4, 1
        do
            channels[chan]["intensity"]["select"] = 0
            channels[chan]["intensity"]["value"] = 176
            channels[chan]["intensity"]["gateoff"] = true
            channels[chan]["intensity"]["max"] = math.random(224,255)
            channels[chan]["intensity"]["rate"] = math.random(6,63)
            channels[chan]["intensity"]["actionmax"] = 255
        end
        channels[1]["blocktimer"]["select"] = 1
        channels[1]["blocktimer"]["rate"] = math.random(5,24) * 256 / 8
        channels[1]["blocktimer"]["nextblock"] = math.random(block_torment1,block_torment3)
    elseif (channels["block"] == block_torment1) then
        for chan = 1, 4, 1
        do
            channels[chan]["intensity"]["select"] = 1
            channels[chan]["intensity"]["gateoff"] = false
            channels[chan]["intensity"]["actionmax"] = block_torment
        end
    elseif (channels["block"] == block_torment2) then
        for chan = 2, 4, 2
        do
            channels[chan]["intensity"]["select"] = 1
            channels[chan]["intensity"]["gateoff"] = false
            channels[chan]["intensity"]["actionmax"] = block_torment
        end
    elseif (channels["block"] == block_torment3) then
        for chan = 1, 3, 2
        do
            channels[chan]["intensity"]["select"] = 1
            channels[chan]["intensity"]["gateoff"] = false
            channels[chan]["intensity"]["actionmax"] = block_torment
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
        for chan = 1, 2, 1
        do
            channels[chan]["width"]["min"] = 50
            channels[chan]["width"]["value"] = channels[chan]["width"]["min"]
            channels[chan]["width"]["steps"] = 4
            channels[chan]["width"]["rate"] = 1
            channels[chan]["freq"]["value"] = 135 -- MA knob
        end
        channels[1]["width"]["select"] = 1
        channels[2]["width"]["select"] = 0
        channels[1]["width"]["actionmax"] = block_orgasm2
    elseif (channels["block"] == block_orgasm2) then
        channels[1]["width"]["steps"] = -1
        channels[1]["width"]["actionmin"] = block_orgasm3
        channels[2]["width"]["select"] = 1
        channels[2]["width"]["actionmax"] = 255
        channels[1]["width"]["min"] = (channels[1]["width"]["min"] + 2)%256
--        print("width",channels[1]["width"]["value"],channels[1]["width"]["min"])
        channels[2]["width"]["min"] = (channels[2]["width"]["min"] + 2)%256
    elseif (channels["block"] == block_orgasm3) then
        channels[1]["width"]["select"] = 0
        channels[2]["width"]["actionmin"] = block_orgasm4
    elseif (channels["block"] == block_orgasm4) then
        channels[1]["width"]["select"] = 1
        channels[2]["width"]["select"] = 0
        channels[1]["width"]["steps"] = 1
        channels[2]["width"]["steps"] = 1
    elseif (channels["block"] == block_random2) then
        for chan = 1, 4, 1
        do
          channels[chan]["intensity"]["select"] = 1
          channels[chan]["intensity"]["rate"] = math.random(1,4)*8
          channels[chan]["freq"]["select"] = 1
          channels[chan]["freq"]["rate"] = math.random(1,4)*8
          channels[chan]["width"]["select"] = 1
          channels[chan]["width"]["rate"] = math.random(1,4)
        end
        for chan = 2, 4, 2
        do
          channels[chan]["freq"]["step"] = math.random(1,4)
        end
        channels[1]["blocktimer"]["select"] = 1
        channels[1]["blocktimer"]["rate"] = math.random(5,31) * 256 / 8
        channels[1]["blocktimer"]["nextblock"] = block_random2
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
    if b["timer"] >= b["rate"] then
         b["timer"] = 0
         b["value"] = b["value"] + b["steps"]
         if b["value"] > b["max"] then
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

function at244hz(nchannels)
    nchannels = nchannels or 4 -- old default
    for chan = 1, nchannels, 1
    do
        ettot.handleblocktimer(channels[chan]["blocktimer"])
        ettot.handlegatetimer(channels[chan]["gate"])

        ettot.handleblock(channels[chan]["freq"])
        if channels[chan]["freq"]["changed"] then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
           if (nchannels == 2) then
               zc.SetFrequency(chan+2, 3750/channels[chan]["freq"]["value"])
           end
           channels[chan]["freq"]["changed"] = false
        end

        ettot.handleblock(channels[chan]["width"])
        if channels[chan]["width"]["changed"] then
            zc.SetPulseWidth(chan, channels[chan]["width"]["value"], channels[chan]["width"]["value"])
            if (nchannels == 2) then
                zc.SetPulseWidth(chan, channels[chan]["width"]["value"], channels[chan]["width"]["value"])
            end
            channels[chan]["width"]["changed"] = false
        end

        ettot.handleblock(channels[chan]["intensity"])
        if (channels[chan]["intensity"]["changed"] or channels[chan]["gate"]["changed"]) then
            local power = channels[chan]["intensity"]["value"]*1000/255
            if (channels[chan]["intensity"]["gateoff"] or channels[chan]["gate"]["gateoff"]) then
               power = 0
            end
            zc.SetPower(chan, power)
            if (nchannels == 2) then
                zc.SetPower(chan+2, power)
            end
            channels[chan]["intensity"]["changed"] = false
            channels[chan]["gate"]["changed"] = false
        end
    end
end
