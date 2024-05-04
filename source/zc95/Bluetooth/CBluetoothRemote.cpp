/*
 * ZC95
 * Copyright (C) 2024  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/* 
 * Deal with input from small bluetooth remotes - the kind often sold as 
 * shutter remotes for phones. Some of them have a D-Pad plus a couple of
 * extra buttons, some just have a shutter button.
 * 
 * Unfortunately, the directional buttons don't just send a simple keypress -
 * they try to simulate motion on a touch screen. So instead of "left button",
 * you get something to indicate the start (in range, tip switch or primary 
 * button pressed), then some x/y position updates in the appropriate direction,
 * then reverse of the start event (i.e. same thing, but with value = 0).
 * 
 * The "shutter button" is the simplest, it sends vol up or vol down as that 
 * triggers the shutter in the iOS camera app (and presumably Android too).
 * 
 * Whenever something is interpreted as a keypress, put it on the keypress 
 * queue.
 */
 
#include "CBluetoothRemote.h"

CBluetoothRemote::CBluetoothRemote()
{
    printf("CBluetoothRemote()\n");
    _movement_started = false;
    _bt_keypress_queue = NULL;
}

CBluetoothRemote::~CBluetoothRemote()
{
    printf("~CBluetoothRemote()\n");
    _bt_keypress_queue = NULL;
}

void CBluetoothRemote::set_keypress_queue(queue_t *bt_keypress_queue)
{
    _bt_keypress_queue = bt_keypress_queue;
}

void CBluetoothRemote::reset_dimension(dimension_t &dimension)
{
    dimension.prev_val = 0;
    dimension.most_recent = 0;
    dimension.received_count = 0;
}

void CBluetoothRemote::process_input(uint16_t usage_page, uint16_t usage, int32_t value)
{
    bool debug = false;

    switch(usage_page)
    {
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            if (debug) print_desktop_page(usage, value);
            process_desktop_page(usage, value);
            break;

        case HID_USAGE_PAGE_BUTTON:
            if (debug) print_button_page(usage, value);
            break;

        case HID_USAGE_PAGE_DIGITIZER:
            if (debug) print_digitizer_page(usage, value);
            break;

        case HID_USAGE_PAGE_CONSUMER:
            if (debug) print_consumer_page(usage, value);
            break;

        default:
            // printf("CBluetoothRemote::process_input(): Unexpected usage page: 0x%X (usage = %d, value = %ld)\n", usage_page, usage, value);
            break;
    }

    if (is_shutter_button(usage_page, usage, value))
    {
        keypress_t key = keypress_t::KEY_SHUTTER;
        send_keypress(key);
    }

    else if (is_movement_start(usage_page, usage, value))
    {
        _movement_started = true;
        
        _x.received_count = 0;
        _y.received_count = 0;
    }

    else if (is_movement_end(usage_page, usage, value))
    {
        _movement_started = false;
        keypress_t key = get_last_direction_button_pressed();
        send_keypress(key);
    }
}

void CBluetoothRemote::send_keypress(keypress_t key)
{
    // If we've had a keypress in the last 150ms, it's really unlikely to be another one,
    // we're far more likely to detecting the same button press as two different things.
    if (time_us_64() - _last_button_event < 1000 * 150)
        return;
    
    printf("CBluetoothRemote: %s\n", s_get_keypress_string(key).c_str());

    _last_button_event = time_us_64();
    if (_bt_keypress_queue)
    {
        bt_keypress_queue_entry_t q;
        q.key = key;
        queue_try_add(_bt_keypress_queue, &q);
    }
}

bool CBluetoothRemote::is_movement_start(uint16_t usage_page, uint16_t usage, int32_t value)
{
    if (_movement_started)
        return false;

    return (value == 1 && is_movement_event(usage_page, usage));
}

bool CBluetoothRemote::is_movement_end(uint16_t usage_page, uint16_t usage, int32_t value)
{
    if (!_movement_started)
        return false;

    return (value == 0 && is_movement_event(usage_page, usage));
}

// Different buttons seem to use different things to indicate the start/end of a left/right/up/down swipe,
// and some send more than one.
// So look for various different things.
bool CBluetoothRemote::is_movement_event(uint16_t usage_page, uint16_t usage)
{
    if (usage_page == HID_USAGE_PAGE_BUTTON     && usage == HID_USAGE_BUTTON_PRIMARY        )  return true;
    if (usage_page == HID_USAGE_PAGE_DIGITIZER  && usage == HID_USAGE_DIGITIZER_TIP_SWITCH  )  return true;
    if (usage_page == HID_USAGE_PAGE_DIGITIZER  && usage == HID_USAGE_DIGITIZER_IN_RANGE    )  return true;

    return false;
}

bool CBluetoothRemote::is_shutter_button(uint16_t usage_page, uint16_t usage, int32_t value)
{
    if (value != 1)
        return false;

    return
    (
        (usage_page == HID_USAGE_PAGE_CONSUMER) &&
        (usage == HID_USAGE_CONSUMER_VOL_DEC || usage == HID_USAGE_CONSUMER_VOL_INC)
    );
}

void CBluetoothRemote::process_desktop_page(uint16_t usage, int32_t value)
{
    if (usage == HID_USAGE_GENERIC_DESKTOP_X)
    {
        _x.received_count++;
        _x.prev_val = _x.most_recent;
        _x.most_recent = value;
    }
    else if (usage == HID_USAGE_GENERIC_DESKTOP_Y)
    {
        _y.received_count++;
        _y.prev_val = _y.most_recent;
        _y.most_recent = value;
    }
}

CBluetoothRemote::keypress_t CBluetoothRemote::get_last_direction_button_pressed()
{
    int32_t x_move = 0;
    int32_t y_move = 0;

    if (_x.received_count < 2 && _y.received_count < 2 )
        return keypress_t::KEY_BUTTON;

    // For remotes that are doing something sensible: movement start, 2 
    // or more x/y updates (hopefully with only one changing), movement end.
    // This works well for 3 of 4 remotes with up/down buttons I have.
    x_move = _x.most_recent - _x.prev_val;
    y_move = _y.most_recent - _y.prev_val;

    if (y_move == 0 && x_move == 0)
    {
        // A bit of weird case - we've received at least 2 x/y updates, but there was no apparent movement based
        // off those two x/y's received.  Take a crude guess at what button was pressed - if only one (x or y) is 
        // non-zero, assume that's the direction of movement.
        // Applies to 1 of the 4 remotes I have with up/down buttons

        if ((_x.most_recent == 0 && _y.most_recent == 0) || 
            (_x.most_recent != 0 && _y.most_recent != 0))
            return keypress_t::KEY_UNKNOWN; // give up, it's probably motion, but can't figure out what

        if (_x.most_recent > 0)
            return keypress_t::KEY_LEFT;
        else if (_x.most_recent < 0)
            return keypress_t::KEY_RIGHT;
        else if (_y.most_recent > 0)
            return keypress_t::KEY_UP;
        else if (_y.most_recent < 0)
            return keypress_t::KEY_DOWN;
        else
            return keypress_t::KEY_UNKNOWN; 
    } 

    // Try and determine which direction the button is trying to move in most. 
    // Ideally, either X or Y move would be 0 making it obvious.
    if (abs(x_move) > abs(y_move))
    {
        // largest movement in x => left/right
        if (x_move < 0)
            return keypress_t::KEY_LEFT;
        else
            return keypress_t::KEY_RIGHT;
    }
    else
    {
        // largest movement in y => up/down
        if (y_move < 0)
            return keypress_t::KEY_UP;
        else
            return keypress_t::KEY_DOWN;
    }
}

std::string CBluetoothRemote::s_get_keypress_string(keypress_t key)
{
    std::string key_string = "<UNKNOWN>";
    switch(key)
    {
        case keypress_t::KEY_BUTTON:
            key_string = "KEY_BUTTON";
            break;

        case keypress_t::KEY_UP:
            key_string = "KEY_UP";
            break;

        case keypress_t::KEY_DOWN:
            key_string = "KEY_DOWN";
            break;

        case keypress_t::KEY_LEFT:
            key_string = "KEY_LEFT";
            break;

        case keypress_t::KEY_RIGHT:
            key_string = "KEY_RIGHT";
            break;

        case keypress_t::KEY_UNKNOWN:
            key_string = "KEY_UNKNOWN";
            break;

        case keypress_t::KEY_SHUTTER:
            key_string = "KEY_SHUTTER";
            break;            
    }

    return key_string;
}

//////////////////
// Debug output //
//////////////////

void CBluetoothRemote::print_desktop_page(uint16_t usage, int32_t value)
{
    switch(usage)
    {
        case HID_USAGE_GENERIC_DESKTOP_X:
            printf("HID_USAGE_GENERIC_DESKTOP_X:            %ld\n", value);
            break;

        case HID_USAGE_GENERIC_DESKTOP_Y:
            printf("HID_USAGE_GENERIC_DESKTOP_Y:            %ld\n", value);
            break;

        case HID_USAGE_GENERIC_DESKTOP_WHEEL:
            printf("HID_USAGE_GENERIC_DESKTOP_WHEEL:        %ld\n", value);
            break;

        default:
            printf("generic desktop: 0x%X = %ld\n", usage, value);
            break;
    }
}

void CBluetoothRemote::print_button_page(uint16_t usage, int32_t value)
{
    switch(usage)
    {
        case HID_USAGE_BUTTON_PRIMARY:
            printf("HID_USAGE_BUTTON_PRIMARY:               %ld\n", value);
            break;

        default:
            printf("button page:                            0x%X = %ld\n", usage, value);
            break;
    }
}

void CBluetoothRemote::print_digitizer_page(uint16_t usage, int32_t value)
{
    switch(usage)
    {
        case HID_USAGE_DIGITIZER_IN_RANGE:
            printf("HID_USAGE_DIGITIZER_IN_RANGE:           %ld\n", value);
            break;

        case HID_USAGE_DIGITIZER_TIP_SWITCH:
            printf("HID_USAGE_DIGITIZER_TIP_SWITCH:         %ld\n", value);
            break;

        case HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER:
            printf("HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER: %ld\n", value);
            break;

        case HID_USAGE_DIGITIZER_CONTACT_COUNT:
            printf("HID_USAGE_DIGITIZER_CONTACT_COUNT:      %ld\n", value);
            break;

        default:
            printf("digitizer: 0x%X = %ld\n", usage, value);
            break;
    }
}

void CBluetoothRemote::print_consumer_page(uint16_t usage, int32_t value)
{
    switch(usage)
    {
        case HID_USAGE_CONSUMER_UNDEFINED:
            printf("HID_USAGE_CONSUMER_UNDEFINED:           %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_POWER:
            printf("HID_USAGE_CONSUMER_POWER:               %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_SCAN_PREV_TRACK:
            printf("HID_USAGE_CONSUMER_SCAN_PREV_TRACK:     %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_EJECT:
            printf("HID_USAGE_CONSUMER_EJECT:               %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_PLAY_PAUSE:
            printf("HID_USAGE_CONSUMER_PLAY_PAUSE:          %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_VOL_INC:
            printf("HID_USAGE_CONSUMER_VOL_INC:             %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_VOL_DEC:
            printf("HID_USAGE_CONSUMER_VOL_DEC:             %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_MUTE:
            printf("HID_USAGE_CONSUMER_MUTE:                %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_AL_KEY_LAYOUT:
            printf("HID_USAGE_CONSUMER_AL_KEY_LAYOUT:       %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_AL_SCREEN_SAVER:
            printf("HID_USAGE_CONSUMER_AL_SCREEN_SAVER:     %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_AC_SEARCH:
            printf("HID_USAGE_CONSUMER_AC_SEARCH:           %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_AC_HOME:
            printf("HID_USAGE_CONSUMER_AC_HOME:             %ld\n", value);
            break;

        case HID_USAGE_CONSUMER_AC_PAN:
            printf("HID_USAGE_CONSUMER_AC_PAN:              %ld\n", value);
            break;

        default:
            printf("consumer page:                          0x%x = %ld\n", usage, value);
            break;
    }
}
