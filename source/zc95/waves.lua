_freq_min_hz = 25
_freq_max_hz = 250
_pulse_min_us = 40
_pulse_max_us = 200

_initial_duration_sec = 25

Config = {
    name = "Waves",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Chan 1 duration",
            id = 1,
            min = 1,
            max = 60,
            increment_step = 1,
            uom = "sec",
            default = _initial_duration_sec
         },
         {
            type = "MIN_MAX",
            title = "Chan 2 duration",
            id = 2,
            min = 1,
            max = 60,
            increment_step = 1,
            uom = "sec",
            default = _initial_duration_sec + 5
         },
         {
            type = "MIN_MAX",
            title = "Chan 3 duration",
            id = 3,
            min = 1,
            max = 60,
            increment_step = 1,
            uom = "sec",
            default = _initial_duration_sec + 10
         },
         {
            type = "MIN_MAX",
            title = "Chan 4 duration",
            id = 4,
            min = 1,
            max = 60,
            increment_step = 1,
            uom = "sec",
            default = _initial_duration_sec + 15
         }
    }
}

function Setup(time_ms)
    print("SETUP")

    channels = {}    
    for chan = 1, 4, 1
    do
        channels[chan] = {}
        channels[chan]["freq"] = {}
        channels[chan]["freq"]["current_position"] = 0 -- cycles between 0 -> 1 -> 0 -> 1 etc (fractional)
        channels[chan]["freq"]["last_update_ms"] = time_ms
        channels[chan]["freq"]["increasing"] = 0
        channels[chan]["freq"]["duration_ms"] = _initial_duration_sec * 1000

        channels[chan]["us"] = {}
        channels[chan]["us"]["current_position"] = 0 
        channels[chan]["us"]["last_update_ms"] = time_ms
        channels[chan]["us"]["increasing"] = 0
        channels[chan]["us"]["duration_ms"] = _initial_duration_sec * 900

        zc.ChannelOn(chan)
        zc.SetPower(chan, 1000)
    end
end

function update_progress_for_channel(current_time_ms, channel)
    local elapsed_ms = current_time_ms - channel["last_update_ms"]
    channel["last_update_ms"] = current_time_ms

    local progress_made = elapsed_ms / channel["duration_ms"]
    local new_position = 0
    
    if channel["increasing"] then
        new_position = channel["current_position"] + progress_made
        if new_position > 1 then
            new_position = 1
            channel["increasing"] = false
        end
    else
        -- decreasing
        new_position = channel["current_position"] - progress_made
        if new_position < 0 then
            new_position = 0
            channel["increasing"] = true
        end
    end

    channel["current_position"] = new_position
end

function get_wave_value_for_channel(channel, current_time_ms, min_value, max_value)
    update_progress_for_channel(current_time_ms, channel)
    return (channel["current_position"] * (max_value - min_value)) + min_value
end

function MinMaxChange(menu_id, min_max_val)
    channels[menu_id]["freq"]["duration_ms"] = min_max_val * 1000
    channels[menu_id]["us"]["duration_ms"]   = min_max_val * 900 -- slightly different so not always in sync
end

function Loop(time_ms)
    for chan = 1, 4, 1
    do
        new_freq_hz = get_wave_value_for_channel(channels[chan]["freq"], time_ms, _freq_min_hz, _freq_max_hz)
        zc.SetFrequency(chan, new_freq_hz)

        new_pulse_us = get_wave_value_for_channel(channels[chan]["us"], time_ms, _pulse_min_us, _pulse_max_us)
        zc.SetPulseWidth(chan, new_pulse_us, new_pulse_us) 
    end
end
