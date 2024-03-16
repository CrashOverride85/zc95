_pulse_len_ms = 100

Config = {
    name = "BtFire",
    bluetooth_remote_passthrough = true,
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Pulse length",
            id = 1,
            min = 100,
            max = 5000,
            increment_step = 100,
            uom = "ms",
            default = _pulse_len_ms
         }
    }
}

function Loop(time_ms)

end

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _pulse_len_ms = min_max_val
    end
end

function BluetoothRemoteKeypress (key)
    if key == "KEY_LEFT" then
        zc.ChannelPulseMs(1, _pulse_len_ms)
    elseif key == "KEY_UP" then
        zc.ChannelPulseMs(2, _pulse_len_ms)
    elseif key == "KEY_DOWN" then
        zc.ChannelPulseMs(3, _pulse_len_ms)
    elseif key == "KEY_RIGHT" then
        zc.ChannelPulseMs(4, _pulse_len_ms)
    elseif key == "KEY_SHUTTER" then
        zc.ChannelPulseMs(1, _pulse_len_ms)
        zc.ChannelPulseMs(2, _pulse_len_ms)
        zc.ChannelPulseMs(3, _pulse_len_ms)
        zc.ChannelPulseMs(4, _pulse_len_ms)
    end
end
