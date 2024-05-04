_value_limit = 255
_last_received_value = 0

Config = {
    name = "BtHid",
    menu_items = {
    }
}

function Setup()
    for chan = 1, 4, 1
    do
        zc.ChannelOn(chan)
        zc.SetPower(chan, 0)
    end
end

function Loop(time_ms)

end

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _value_limit = min_max_val
        SetPowerLevel()
    end
end

function BluetoothHidEvent (usage_page, usage, value)
    if (usage_page == 0xFF00 and usage == 0x01)
    then
        _last_received_value = value
        SetPowerLevel()
    end
end

function SetPowerLevel()
    power_level = (1000/_value_limit) * _last_received_value
    if (power_level > 1000)
    then
        power_level = 1000
    end

    for chan = 1, 4, 1
    do
        zc.ChannelOn(chan)
        zc.SetPower(chan, power_level)
    end
end
