_delay_ms = 500

Config = {
    name = "Toggle",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Delay",
            id = 1,
            min = 100,
            max = 2000,
            increment_step = 100,
            uom = "ms",
            default = _delay_ms
         },
         {
            type = "MULTI_CHOICE",
            title = "Output",
            id = 2,
            choices = {
                {choice_id = 1, description = "Pulse"},
                {choice_id = 2, description = "Constant"}
            }
         }
    }
}

_channel12_on = false
_wait_until_ms = 0
_pulse_mode = true

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _delay_ms = min_max_val
    end
end

function MultiChoiceChange(menu_id, choice_id)
    if (menu_id == 2 and choice_id == 1)
    then
        _pulse_mode = true
    end

    if (menu_id == 2 and choice_id == 2)
    then
        _pulse_mode = false
    end
end

function Loop(time_ms)
    if (time_ms > _wait_until_ms)
    then
        ToggleChannel();
        _wait_until_ms = time_ms + _delay_ms
    end
end

function ToggleChannel()
    if (_channel12_on == true)
    then
        zc.ChannelOff(1)
        zc.ChannelOff(2)

        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(3, 100)
            zc.ChannelPulseMs(4, 100)
        else
            zc.ChannelOn(3)
            zc.ChannelOn(4)
        end
        _channel12_on = false
    else
        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(1, 100)
            zc.ChannelPulseMs(2, 100)
        else
            zc.ChannelOn(1)
            zc.ChannelOn(2)
        end

        zc.ChannelOff(3)
        zc.ChannelOff(4)
        _channel12_on = true
    end
end
