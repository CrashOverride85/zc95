Config = {
    name = "Fire"
}

function Loop(time_ms)

end

function ExternalTrigger(socket, part, active)
    channel = 0

    if socket == "TRIGGER1" then
        if part == "A" then
            channel = 1
        else -- "B"
            channel = 2
        end
    else -- "TRIGGER2"
        if part == "A" then
            channel = 3
        else -- "B"
            channel = 4
        end
    end

    if active then
        zc.ChannelOn(channel)
    else
        zc.ChannelOff(channel)
    end
end

function SoftButton(pushed)
    AllChannels(pushed)    
end

function AllChannels(on)
    for chan = 1, 4, 1
    do
        if on then
            zc.ChannelOn(chan)
        else
            zc.ChannelOff(chan)
        end
    end
end
