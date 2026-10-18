-- Library to handle ET modes - code kindly contributed by someone who perfers to remain anonymous

module("ettot", package.seeall)

-- Defaults for all the modes
function setupdefaults(n)
    n["intensity"] = {}
    n["intensity"]["value"] = 255
    n["intensity"]["min"] = 205
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
    n["freq"]["select"] = 0 -- default is use MA value
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
    n["width"]["select"] = 0 -- default is use Advanced Param value
    n["width"]["timer"] = 0
    n["width"]["changed"] = true

    n["gate"] = {}
    n["gate"]["select"] = 0
    n["gate"]["timer"] = 0 -- on/off time is 62 default
    n["gate"]["rate"] = 0
    n["gate"]["gateoff"] = false
    n["gate"]["changed"] = true

    n["blocktimer"] = {}
    n["blocktimer"]["select"] = 0
    n["blocktimer"]["timer"] = 0 -- max is 255 default
    n["blocktimer"]["rate"] = 0
    n["blocktimer"]["nextblock"] = 0

    -- ET gate value bits 2/1: 1 = phase 1 (positive) only, 2 = phase 2 (negative) only, 3 = both
    n["phases"] = 3
end

-- The firmware's clocks. A block, gate or block timer picks one with ["clock"] (select bits 0-1):
-- 1 = $4088 244 Hz (default), 2 = $408b 30.5 Hz (ticks when $4088 & 7 == 7), 3 = $4089 0.953 Hz
-- (ticks when $4088 wraps). Each keeps the clock value at its last update in ["timer"] and updates
-- when (clock - timer) & 0xff >= rate, storing clock + 1, so it runs every rate+1 ticks of its clock.
clock = {0, 0, 0}

function advanceclock()
    clock[1] = (clock[1] + 1) % 256
    if clock[1] % 8 == 7 then clock[2] = (clock[2] + 1) % 256 end
    if clock[1] == 0 then clock[3] = (clock[3] + 1) % 256 end
end

-- check_selected_routine_timer_tick (flash 0x31c) and the rate check that follows it
function due(b, rate)
    local c = b["clock"] or 1
    if c == 2 and clock[1] % 8 ~= 7 then return false end
    if c == 3 and clock[1] ~= 0 then return false end
    if (clock[c] - b["timer"]) % 256 < rate then return false end
    b["timer"] = (clock[c] + 1) % 256
    return true
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
             channels[chan]["intensity"]["actionmin"] = 254 -- 0xfe: reverse and swap phase 1/2 (gate xor 0x06)
             channels[chan]["intensity"]["actionmax"] = 254
             channels[chan]["phases"] = 1 -- modules 3/4 set gate 0x05: phase 1 only
             channels[chan]["width"]["value"] = 255
          end

          for chan = 2, 4, 2
          do
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
            channels[chan]["gate"]["rate"] = 12 -- MA
          end
          channels[1]["blocktimer"]["select"] = 1
          channels[1]["blocktimer"]["clock"] = 2
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
          for chan = 1, 4, 1
          do
              channels[chan]["gate"]["select"] = 1
              channels[chan]["gate"]["clock"] = 2
              channels[chan]["gate"]["rate"] = 32 -- MA knob
              channels[chan]["freq"]["select"] = 1
              channels[chan]["freq"]["clock"] = 2
              channels[chan]["freq"]["rate"] = 1 -- default
              channels[chan]["width"]["select"] = 1
              channels[chan]["width"]["clock"] = 2
              channels[chan]["width"]["rate"] = 5 -- advanced param (pace)
              channels[chan]["width"]["value"] = 130 -- advanced param 130
          end
          for chan = 2, 4, 2
          do
              channels[chan]["freq"]["steps"] = 2
              channels[chan]["width"]["steps"] = 2
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
        channels[1]["blocktimer"]["clock"] = 3
        channels[1]["blocktimer"]["rate"] = math.random(5,24)
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
        for chan = 1, 2, 1
        do
          channels[chan]["freq"]["select"] = 1
          channels[chan]["freq"]["rate"] = 50  -- MA knob 1 to 100
          channels[chan]["freq"]["value"] = 255
          channels[chan]["freq"]["max"] = 255
          channels[chan]["freq"]["steps"] = 1
        end
        -- on the box A reaches min first and module 6 runs on both channels, so B follows
        -- A's chain 5 -> 6 -> 7 in step; B's own chain (8 -> 9 -> 10) only runs in split mode
        channels[1]["freq"]["actionmin"] = block_climb6
        channels[2]["freq"]["actionmin"] = block_climb6
    elseif (channels["block"] == block_climb5) then
        for chan = 1, 2, 1
        do
          channels[chan]["freq"]["select"] = 1
          channels[chan]["freq"]["value"] = 255
          channels[chan]["freq"]["max"] = 255
          channels[chan]["freq"]["steps"] = 1
          channels[chan]["freq"]["actionmin"] = block_climb6
        end
    elseif (channels["block"] == block_climb6) then
        for chan = 1, 2, 1
        do
          channels[chan]["freq"]["value"] = 255
          channels[chan]["freq"]["steps"] = 2
          channels[chan]["freq"]["actionmin"] = block_climb7
        end
    elseif (channels["block"] == block_climb7) then
        for chan = 1, 2, 1
        do
          channels[chan]["freq"]["value"] = 255
          channels[chan]["freq"]["steps"] = 4
          channels[chan]["freq"]["actionmin"] = block_climb5
        end
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
                channels[chan]["intensity"]["min"] = 215 -- advanced param
                channels[chan]["intensity"]["rate"] = 1 -- advanced param
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

        channels[1]["width"]["min"] = (channels[1]["width"]["min"] + 2) %256
        channels[2]["width"]["min"] = (channels[2]["width"]["min"] + 2) %256
        print("block 25 width2 min is",channels[2]["width"]["min"])
    elseif (channels["block"] == block_orgasm3) then
        channels[1]["width"]["select"] = 0
        channels[2]["width"]["actionmin"] = block_orgasm4
    elseif (channels["block"] == block_orgasm4) then
        channels[1]["width"]["select"] = 1
        channels[2]["width"]["select"] = 0
        channels[1]["width"]["steps"] = 1
        channels[2]["width"]["steps"] = 1
    elseif (channels["block"] == block_random2) then
        -- The module sets the random range to 1-4, picks the rates, then sets 5-31 for the
        -- block timer. Its random opcode always reads channel A's range ($408d/$408e), so on
        -- the channel B pass the picks come from the 5-31 left behind: B gets rates (and a
        -- frequency step) of 5-31. A's frequency step is not changed.
        for chan = 1, 4, 1
        do
          local lo, hi = 1, 4
          if chan % 2 == 0 then
            lo, hi = 5, 31
            channels[chan]["freq"]["steps"] = math.random(lo,hi)
          end
          channels[chan]["intensity"]["select"] = 1
          channels[chan]["intensity"]["clock"] = 2
          channels[chan]["intensity"]["rate"] = math.random(lo,hi)
          channels[chan]["freq"]["select"] = 1
          channels[chan]["freq"]["clock"] = 2
          channels[chan]["freq"]["rate"] = math.random(lo,hi)
          channels[chan]["width"]["select"] = 1
          channels[chan]["width"]["rate"] = math.random(lo,hi)
        end
        channels[1]["blocktimer"]["select"] = 1
        channels[1]["blocktimer"]["clock"] = 3
        channels[1]["blocktimer"]["rate"] = math.random(5,31)
        channels[1]["blocktimer"]["nextblock"] = block_random2
     end
end

function handleblocktimer(b)
    if b["select"] == 0 then
      return
    end
    if due(b, b["rate"]) then
      channels["block"] = b["nextblock"]
      setupblock(channels)
    end
end

function handlegatetimer(b)
    if b["select"] == 0 then
      return
    end
    if due(b, b["rate"]) then
      b["gateoff"] = not b["gateoff"]
      b["changed"] = true
    end
end

function swapphases(chan)
    channels[chan]["phases"] = 3 - channels[chan]["phases"] -- xor 3
    channels[chan]["width"]["changed"] = true
end

function handleblock(b,name,xchan)
    local changed = false

    if b["select"] == 0 then
       return false
    end
    if due(b, b["rate"]) then
         b["value"] = b["value"] + b["steps"]
--         print(name,xchan,"adding ",b["steps"]," to ",b["value"]," min ",b["min"]," max ",b["max"])
         if (b["steps"]>=0 and b["value"] > b["max"]) then
--             print(name,xchan,"is bigger than max")
             b["value"] = b["max"]
             if b["actionmax"] == 255 then
                 b["steps"] = -b["steps"]
             elseif b["actionmax"] == 254 then
                 b["steps"] = -b["steps"]
                 swapphases(xchan)
             elseif b["actionmax"] == 253 then
                 b["value"] = b["min"]
             elseif b["actionmax"] <200 then
                 channels["block"] = b["actionmax"]
                 setupblock(channels)
             end
         elseif (b["steps"]<=0 and b["value"] <= b["min"]) then
--            print(name,xchan,"is smaller than min")
             b["value"] = b["min"]
             if b["actionmin"] == 255 then
                 b["steps"] = -b["steps"]
             elseif b["actionmin"] == 254 then
                 b["steps"] = -b["steps"]
                 swapphases(xchan)
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

function setpower(chan, level)
    if (level == 0) 
    then
        zc.ChannelOff(chan)
        zc.SetPower(chan, 0)
    else
        zc.SetPower(chan, level)
        zc.ChannelOn(chan)
    end
end

function at244hz(nchannels)
    nchannels = nchannels or 4 -- old default
    local chan
    advanceclock()
    for chan = 1, nchannels, 1
    do
        ettot.handleblocktimer(channels[chan]["blocktimer"])
        ettot.handlegatetimer(channels[chan]["gate"])

        ettot.handleblock(channels[chan]["freq"],"f",chan)
        if channels[chan]["freq"]["changed"] then
           zc.SetFrequency(chan, 3750/channels[chan]["freq"]["value"])
           if (nchannels == 2) then
               zc.SetFrequency(chan+2, 3750/channels[chan]["freq"]["value"])
           end
           channels[chan]["freq"]["changed"] = false
        end

        ettot.handleblock(channels[chan]["width"],"w",chan)

        ettot.handleblock(channels[chan]["intensity"],"i",chan)
        if (channels[chan]["intensity"]["changed"] or channels[chan]["gate"]["changed"]) then
            local power = channels[chan]["intensity"]["value"]*1000/255
            if (channels[chan]["intensity"]["gateoff"] or channels[chan]["gate"]["gateoff"]) then
               power = 0
            end
            setpower(chan, power)
            if (nchannels == 2) then
                setpower(chan+2, power)
            end
            channels[chan]["intensity"]["changed"] = false
            channels[chan]["gate"]["changed"] = false
        end

        -- after intensity, so a phase swap from its 0xfe action is applied in the same tick
        if channels[chan]["width"]["changed"] then
            local w = channels[chan]["width"]["value"]
            local p = channels[chan]["phases"]
            local pos, neg = 0, 0
            if p == 1 or p == 3 then pos = w end
            if p >= 2 then neg = w end
            zc.SetPulseWidth(chan, pos, neg)
            if (nchannels == 2) then
                zc.SetPulseWidth(chan+2, pos, neg)
            end
            channels[chan]["width"]["changed"] = false
        end
    end
end
