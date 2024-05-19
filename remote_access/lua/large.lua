nom="NewProg34"

init=true  --Phase d'initialisation
globalTime=0

enema={}
--Config Listbox
    enema["defaultVolume"]=1000 --en ml
    enema["defaultTime"]=31000 --en ms
    enema["selectedVolume"]=1000 --en ml   
    enema["selectedTime"]= 31000  --en ms

    enema["volumeMini"]=500 --en ml
    enema["volumeMaxi"]=4000 --en ml
    enema["volumeIncrement"]=100 --en ml
    enema["1000mlWater"]=31000  --en ms
    enema["100mlAdditive"]=60000 --en ms  
    -- Variables
    enema["currentState"]=false
    enema["selectedTime"]= 31000 
    
    enema["currentVolume"]=0
    enema["currentEnlapsedTime"]=0

    enema["lastOn"]=0
    enema["lastOff"]=0

--additive={}
--additive["currentState"]=false  
--additive["lastOn"]=0    
--additive["lastOff"]=0   

trigger={}
trigger["socket"]="0"
trigger["part"]="0"
trigger["active"]=false
trigger["newCommand"]=false

lock={}
lock["state"]=false
lock["lastOn"]=0
--Config Listbox
lock["minimalLockTime"]=1
lock["defaultLockTime"]=10
lock["currentLockTime"]=10
lock["maximalLockTime"]=120

Config = {
    name = nom,
    menu_items = {
        
        {
            type = "MIN_MAX",
            title = "Lock Time",
            id = 1,
            min = lock["minimalLockTime"],
            max = lock["maximalLockTime"],
            increment_step = 1,
            uom = "min",
            default = lock["defaultLockTime"],
         },
        {
            type = "MIN_MAX",
            title = "Enema Volume",
            id = 2,
            min = enema["volumeMini"],
            max = enema["volumeMaxi"],
            increment_step = enema["volumeIncrement"],
            uom = "ml",
            default = enema["selectedVolume"],
         },
         {
            type = "MIN_MAX",
            title = "Water / 1000ml",
            id = 3,
            min = 10,
            max = 200,
            increment_step = 1,
            uom = "s",
            default = 31,
         },
         {
            type = "MIN_MAX",
            title = "Pump 100ml",
            id = 4,
            min = 5,
            max = 200,
            increment_step = 1,
            uom = "s",
            default = 60,    
         },
         {
            type = "MIN_MAX",
            title = "Puissance Initiale",
            id = 5,
            min = 0,
            max = 1000,
            increment_step = 10,
            uom = "/1000",
            default = 500,    
         },
         {
            type = "MIN_MAX",
            title = "Speed",
            id = 6,
            min = 1,
            max = 64,
            increment_step = 2,
            uom = "",
            default = 32,
         }
    }
}

function Setup(time_ms)
    print("SETUP")
    --globalTime=time_ms

    zc.AccIoWrite(1, true) -- initialize locker relay
    zc.AccIoWrite(2, true) -- initialize enema relay
    zc.AccIoWrite(3, true) -- initialize additive relay

    channels = {}
    
    for chan = 1, 4, 1 do
        channels[chan] = {}

            channels[chan]["power"] = {}
            channels[chan]["power"]["default"] = 500
            channels[chan]["power"]["current"] = 500
            channels[chan]["power"]["shockPower"] = 500
            channels[chan]["power"]["mini"] = 500
            channels[chan]["power"]["incrementAuto"] = 5
            channels[chan]["power"]["currentIncrementInterval"] = 10000
            channels[chan]["power"]["provisoireIntervalle"] = 30000
            channels[chan]["power"]["lastUp"] = 0
            channels[chan]["power"]["lastDown"] = 0

            channels[chan]["shock"] = {}
            channels[chan]["shock"]["duration"] = 500 --durée du shock en ms
            channels[chan]["shock"]["percentElevation"] = 15 --elevation du shock en %
            channels[chan]["shock"]["state"] = false
            channels[chan]["shock"]["lastOn"] = 0


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
        zc.SetPower(chan, channels[chan]["power"]["default"])
    end

    boutonPushed = 0 --Retourne le numéro du bouton actuellement poussé
    bouton = {}
    for button = 1, 4, 1 do
        bouton[button] = {}
        --bouton[button]["pushed"] = false
        bouton[button]["lastPush"] = 0
        bouton[button]["lastPull"] = 0
    end

    
    modes = {}
    selectedMode=1

    for mode = 1, 4, 1 do
        modes[mode] = {}
        modes[mode]["authorized"] = true
        modes[mode]["lastOn"] = 0
        modes[mode]["lastOff"] = 0
    end
end

function MinMaxChange(menu_id, min_max_val)
    if menu_id == 1 then -- Change the lock timer parameter
        lock["currentLockTime"] = min_max_val
        print("Change Val lock time "..lock["currentLockTime"])    
    end
    if menu_id == 2 then -- Change the volume parameter
        enema["selectedVolume"] = min_max_val
        print("Nouveau volume de lavement"..enema["selectedVolume"])    
        enema["selectedTime"]=enema["selectedVolume"]/1000*enema["1000mlWater"] 
        print("Nouveau temps de lavement"..enema["selectedTime"].."ms")   
    end

    if menu_id == 3 then -- Parametrage durée pour 1l d'eau
        enema["1000mlWater"] = min_max_val*1000
        print("Change Val 1000ml Water "..enema["1000mlWater"])
        enema["selectedTime"]=enema["selectedVolume"]/1000*enema["1000mlWater"] 
        print("Nouveau temps de lavement"..enema["selectedTime"].."ms")  
    end

    if menu_id == 4 then -- parametrage durée pour 100ml d'additif
        enema["100mlAdditive"] = min_max_val
        print("Change Val 100ml Additive "..enema["100mlAdditive"]) 
    end

    if menu_id == 5 then -- Change the initial power parameter
        for chan = 1, 4, 1 do
            channels[chan]["power"]["default"] = min_max_val 
            channels[chan]["power"]["current"] = min_max_val 
            zc.SetPower(chan, channels[chan]["power"]["current"])
        end
    end

    if menu_id == 6 then -- Change the speed parameter
        for chan = 1, 4, 1 do
            channels[chan]["freq"]["rate"] = min_max_val
            channels[chan]["us"]["rate"] = min_max_val
        end
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

    globalTime=time_ms

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
    if elapsed_ms >= 5000 then
       _last_ms = time_ms
       print("t")
    --print("tick", elapsed_ms, _loop_count)
       _loop_count = 0
    end

    checkExternalTrigger(time_ms)
    --checkShock(time_ms)
end
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

longPressTime=300 --temps nécessaire pour changer de mode
extralongPressTime=3000 --temps nécessaire pour activer le verrouillage 

function ExternalTrigger(socket, part, active)
    trigger["socket"]=socket
    trigger["part"]=part
    trigger["active"]=active
    trigger["newCommand"]=true
   -- runShock(1, 5000, 15)   
end

function checkExternalTrigger(time_ms)
    if boutonPushed==0 then --Si aucun bouton n'est activé
        --print("Bouton presse")
        if trigger["socket"]== "TRIGGER1" and trigger["active"] then  -- Soit le bouton A ou B est active
            if trigger["part"] == "A"  then  --LE BOUTON A EST ACTIVE
                boutonPushed=1
                bouton[1]["lastPush"]=time_ms   
            end
            if trigger["part"] == "B"  then  --LE BOUTON B EST ACTIVE
                boutonPushed=2
                bouton[2]["lastPush"]=time_ms
            end
        end
        if trigger["socket"] == "TRIGGER2" and trigger["active"] then  -- Soit le bouton A ou B est active
            if trigger["part"] == "A"  then  --LE BOUTON A EST ACTIVE
                boutonPushed=3
                bouton[3]["lastPush"]=time_ms
            end
            if trigger["part"]== "B"  then  --LE BOUTON B EST ACTIVE
                boutonPushed=4
                bouton[4]["lastPush"]=time_ms
            end
        end
    end

    

    if not trigger["active"] then
        --print("Bouton relache")
        if trigger["socket"] == "TRIGGER1" then
            if trigger["part"] == "A" and boutonPushed==1 then
                if (time_ms - bouton[1]["lastPush"])>extralongPressTime then
                    extralongPress(1)
                elseif (time_ms-bouton[1]["lastPush"])>longPressTime then
                    longPress(1)
                else
                    shortPress(1)
                end
                boutonPushed=0
            end
            if trigger["part"] == "B" and boutonPushed==2 then
                if (time_ms-bouton[2]["lastPush"])>extralongPressTime then
                    extralongPress(2)
                elseif (time_ms-bouton[2]["lastPush"])>longPressTime then
                    longPress(2)
                else
                    shortPress(2)
                end
                boutonPushed=0
            end
        elseif trigger["socket"] == "TRIGGER2" then
            if trigger["part"] == "A" and boutonPushed==3 then
                bouton[3]["lastPull"]=time_ms
                if (bouton[3]["lastPull"]-bouton[3]["lastPush"])>extralongPressTime then
                    extralongPress(3)
                elseif (bouton[3]["lastPull"]-bouton[3]["lastPush"])>longPressTime then
                    longPress(3)
                else
                    shortPress(3)
                end
                boutonPushed=0
            end
            if trigger["part"] == "B" and boutonPushed==4 then
                bouton[4]["lastPull"]=time_ms
                if (bouton[4]["lastPull"]-bouton[4]["lastPush"])>extralongPressTime then
                    extralongPress(4)
                elseif (bouton[4]["lastPull"]-bouton[4]["lastPush"])>longPressTime then
                    longPress(4) 
                else           
                    shortPress(4)
                end
                boutonPushed=0
            end
        end

    end
end


--------------- ASSIGNATION DES BOUTONS ----------------

function shortPress(button)
    print("shortPress"..button)
    if selectedMode==1 then
        upPower(button) 
    elseif selectedMode==2 then
        --runShock (button, 3000,15)
        if button==1 then
            for chan = 1, 4, 1 do
               -- checkShock(chan, globalTime)
            end
        end 
    elseif selectedMode==3 then

    elseif selectedMode==4 then

    elseif selectedMode==5 then
        if button==1 then
            if enema["currentState"]==false then
                enemaPlay()
            else
                enemaStop()
            end
        end
    elseif selectedMode==6 then

    elseif selectedMode==7 then

    elseif selectedMode==8 then

    end
end

function longPress(button)

    if selectedMode==button then
        selectedMode=(button+4)
        print("mode "..selectedMode.." selectionne")
    else
        selectedMode=button
        print("mode "..selectedMode.." selectionne")
    end
end



function extralongPress(button)
    print("extralongPress"..button)
    if button == 1 then
        if init then
            lockActivate(true)
            init=false
        else
            lockActivate(false)
        end
        
    elseif button == 2 then
        --runShock(1, 3000, 15)   

    elseif button == 3 then

    elseif button == 4 then
       selectedMode=1
       print("Reset / mode "..selectedMode.." selectionne")
    end
end 


---------------------- FONCTION POWER / IO ----------------------------

function IOactivate (number, state)
    if state then
        zc.AccIoWrite(number, false)
    else
        zc.AccIoWrite(number, true)
    end
end 

function upPower(chan)
   if not channels[chan]["shock"]["state"] then
            channels[chan]["power"]["current"] = channels[chan]["power"]["current"] + channels[chan]["power"]["incrementAuto"]  
            if channels[chan]["power"]["current"] > 1000 then
                channels[chan]["power"]["current"] = 1000
            else
                print("Puissance "..channels[chan]["power"]["current"])
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end  
   end     
end

------------------LOCKER -------------------------------

function lockActivate(state)
    if state then --si un verrouillage est demande
    print("verrouillage demande")
        if lock["state"] then --si le verrou est déjà activé
            print("le verrou est déjà activé    ")
        elseif lock["state"]==false then --si le verrou est ouvert
            IOactivate(1, true)
            lock["state"]=true
            lock["lastOn"]=globalTime
            print("verrouillage active - lets play")
        end
    elseif state==false then --Si une désactivation est demandée
        print("deverrouillage demande")
        if lock["state"] then --Si le verrou est fermé 
            if (globalTime - lock["lastOn"])>lock["currentLockTime"]*60000 or enema["currentEnlapsedTime"]>enema["selectedTime"] then
                IOactivate(1, false)
                lock["state"]=false
                lock["lastOff"]=globalTime
                print("temps ecoulé - verrou desactive")
            else  --Si le trmps de verrouillage n'est pas encore écoulé 
                print("deverrouillage interdit - reste: "..lock["currentLockTime"]*60000-(globalTime - lock["lastOn"]).."ms") 
            end
        else --Si le verrou est ouvert
          print("Verrou deja ouvert")    
        end   
    end 
end

function enemaPlay ()
    if enema["currentState"]==false then
        enema["currentState"]=true
        enema["lastOn"]=globalTime
        enema["lastOff"]=0
        print("Enema play")
        IOactivate(2, true)
    else
        print("Enema already playing")
    end
end 

function enemaStop ()
    if enema["currentState"]==true then
        enema["currentEnlapsedTime"]=enema["currentEnlapsedTime"]+(globalTime-enema["lastOn"])
        print("temps ecoulé de lavement: "..enema["currentEnlapsedTime"])      
        enema["currentState"]=false
        enema["lastOff"]=globalTime
        enema["lastOn"]=0
        print("Enema stop")
        IOactivate(2, false)
    else
        print("Enema already stopped")
    end
end 


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


--[[
function runShock (chan, duration, percentElevation)
    if channels[chan]["shock"]["state"]==false then
        channels[chan]["shock"]["state"]=true
        channels[chan]["shock"]["lastOn"]=globalTime
        channels[chan]["shock"]["lastOff"]=0
        channels[chan]["shock"]["duration"]=duration
        channels[chan]["power"]["shockPower"] = channels[chan]["power"]["current"]*(1+percentElevation/100)
        print("Shock "..chan.." play")
        zc.SetPower(chan, channels[chan]["power"]["shockPower"])
        print("Puissance du shock"..channels[chan]["power"]["shockPower"])
    else
        print("Shock "..chan.." already playing")
    end
end

function checkShock(time)
    for chan = 1, 4, 1 do
        if channels[chan]["shock"]["state"]==true then
            if (time - channels[chan]["shock"]["lastOn"])>channels[chan]["shock"]["duration"] then
                channels[chan]["shock"]["state"]=false
                channels[chan]["shock"]["lastOff"]=time
                print("Shock "..chan.." stop")
                zc.SetPower(chan, channels[chan]["power"]["current"])
            end
        end
    end
end
]]--


