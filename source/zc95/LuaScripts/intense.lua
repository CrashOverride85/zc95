
-- Loosely based on the "intense" pattern of a 312 box, as documented by Onwrikbaar on
-- Joanne's Estim discord (#neostim-chat, 2024/06/04 18:45).
-- Channels 1+2 are on constant with a configurable frequency and the default pulse width (currently 150us) 
-- Channels 3+4 are the same freq/pulse width, but toggles between the two with a configurable toggle time
-- Using just channels 2+3 and not changing the defaults would be similar-ish to running intense on a 312
-- box with the MA dial in the middle.

_toggle_delay_ms = 250
_freq_1_2 = 40
_freq_3_4 = 40

_last_toggle_time_ms = 0
_chan3_on = false

Config = {
    name = "Intense",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Toggle delay",
            id = 1,
            min = 100,
            max = 1000,
            increment_step = 50,
            uom = "ms",
            default = _toggle_delay_ms
         },
         {
            type = "MIN_MAX",
            title = "Chan 1&2 freq",
            id = 2,
            min = 10,
            max = 150,
            increment_step = 5,
            uom = "hz",
            default = _freq_1_2
         },
         {
            type = "MIN_MAX",
            title = "Chan 3&4 freq",
            id = 3,
            min = 10,
            max = 150,
            increment_step = 5,
            uom = "hz",
            default = _freq_3_4
         }
    }
}

function Setup(time_ms)

    for chan = 1, 2, 1
    do
        zc.SetFrequency(chan, _freq_1_2)
        zc.ChannelOn(chan)
        zc.SetPower(chan, 1000)
    end

    for chan = 3, 4, 1
    do
        zc.SetFrequency(chan, _freq_3_4)
        zc.SetPower(chan, 1000)
    end
end

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _toggle_delay_ms = min_max_val
    end

    if (menu_id == 2)
    then
        _freq_1_2 = min_max_val
        zc.SetFrequency(1, _freq_1_2)
        zc.SetFrequency(2, _freq_1_2)
    end

    if (menu_id == 3)
    then
        _freq_3_4 = min_max_val
        zc.SetFrequency(3, _freq_3_4)
        zc.SetFrequency(4, _freq_3_4)
    end
end

function Loop(time_ms)
    if (time_ms > _last_toggle_time_ms + _toggle_delay_ms)
    then
        if (_chan3_on == true)
        then
            zc.ChannelOff(3)
            zc.ChannelOn(4)
            _chan3_on = false
        else
            zc.ChannelOn(3)
            zc.ChannelOff(4)
            _chan3_on = true
        end

        _last_toggle_time_ms = time_ms
    end
end
