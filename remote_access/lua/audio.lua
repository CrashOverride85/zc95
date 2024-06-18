
Config = {
    name = "MicAndConstant",
    audio_processing_mode = "AUDIO_INTENSITY",
    menu_items = {
        {
            type = "AUDIO_VIEW_INTENSITY_MONO",
            title = "Audio",
            id = 1
         }
    }
}

function Setup(time_ms)
    for chan = 1, 4, 1
    do
        zc.ChannelOn(chan)
    end

    zc.SetPower(1, 1000)
    zc.SetPower(2, 1000)
    zc.SetPower(3, 0)
    zc.SetPower(4, 0)
end

function Loop(time_ms)

end

function AudioIntensityChange(left_chan, right_chan, virt_chan)
    left_chan_power = left_chan * 4;

    if (left_chan_power > 1000)
    then
        left_chan_power = 1000
    end

    zc.SetPower(3, left_chan_power)
    zc.SetPower(4, left_chan_power)
end
