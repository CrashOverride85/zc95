#include "CBluetoothRemote.h"

CBluetoothRemote::CBluetoothRemote()
{
    printf("CBluetoothRemote()\n");
    _movement_started = false;
}

CBluetoothRemote::~CBluetoothRemote()
{
    printf("~CBluetoothRemote()\n");
}

void CBluetoothRemote::reset_dimension(dimension_t &dimension)
{
    dimension.prev_val = 0;
    dimension.most_recent = 0;
    dimension.received_count = 0;
    dimension.initial_val = 0;
}

void CBluetoothRemote::process_input(uint16_t usage_page, uint16_t usage, int32_t value)
{
    bool debug = true;

    switch(usage_page)
    {
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            if (debug) print_desktop_page(usage, value);
            process_desktop_page(usage, value);
            break;

        case HID_USAGE_PAGE_BUTTON:
            if (debug) print_button_page(usage, value);
            process_button_page(usage, value);
            break;

        case HID_USAGE_PAGE_DIGITIZER:
            if (debug) print_digitizer_page(usage, value);
            process_digitizer_page(usage, value);
            break;

        case HID_USAGE_PAGE_CONSUMER:
            if (debug) print_consumer_page(usage, value);
            process_consumer_page(usage, value);
            break;

        default:
            printf("CBluetoothRemote::process_input(): Unexpected usage page: 0x%X\n", usage_page);
            break;
    }


    if (is_movement_start(usage_page, usage, value))
    {
        _movement_started = true;

        _x.initial_val = _x.most_recent;
        _y.initial_val = _y.most_recent;
        
        _x.received_count = 0;
        _y.received_count = 0;
    }
    else if (is_movement_end(usage_page, usage, value))
    {
        _movement_started = false;
        keypress_t key = get_last_button_pressed();
        print_keypress(key);
        printf("\n");
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

void CBluetoothRemote::process_desktop_page(uint16_t usage, int32_t value)
{
 //   if (!_movement_started)
 //       return;

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

void CBluetoothRemote::process_button_page(uint16_t usage, int32_t value)
{

    /*
    if (usage == HID_USAGE_BUTTON_PRIMARY)
    {
        if (_pressed && value == 0)
        {
            _pressed = false;
            report_keypress();
        }

        if (!_pressed && value == 1)
        {
            _pressed = true;
            report_keypress();
        }
    }
    */
}

void CBluetoothRemote::process_digitizer_page(uint16_t usage, int32_t value)
{

}

void CBluetoothRemote::process_consumer_page(uint16_t usage, int32_t value)
{
   
}


CBluetoothRemote::keypress_t CBluetoothRemote::get_last_button_pressed()
{
    int32_t x_move = 0;
    int32_t y_move = 0;

    if (_x.received_count < 2 && _y.received_count < 2 )
        return keypress_t::KEY_NONE;


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
            return keypress_t::KEY_NONE; // give up

        if (_x.most_recent > 0)
            return keypress_t::KEY_LEFT;
        else if (_x.most_recent < 0)
            return keypress_t::KEY_RIGHT;
        else if (_y.most_recent > 0)
            return keypress_t::KEY_UP;
        else if (_y.most_recent < 0)
            return keypress_t::KEY_DOWN;
        else
            return keypress_t::KEY_NONE;
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

void CBluetoothRemote::end_of_input()
{

    /*
    bool location_updated = false;
    int x_diff = 0;
    int y_diff = 0;

    if (_current_x != _last_x)
    {
        if (_current_x < _last_x)
            _direction = direction_t::DIR_LEFT;
        else
            _direction = direction_t::DIR_RIGHT;
    
        x_diff = _last_x - _current_x;
        _last_x = _current_x;
        location_updated = true;
    }

    if (_current_y != _last_y)
    {
        if (_current_y < _last_y)
            _direction = direction_t::DIR_DOWN;
        else
            _direction = direction_t::DIR_UP;

        y_diff = _last_y - _current_y;
        _last_y = _current_y;
        location_updated = true;
    }

    if (location_updated)
        printf("\t\t%li\t%li\t\t\t(%d\t%d)\n", _last_x, _last_y, x_diff, y_diff);
        */
}

void CBluetoothRemote::report_keypress()
{
    switch(_direction)
    {
        case DIR_UP:
            printf("KEYPRESS: UP ");
            break;

        case DIR_DOWN:
            printf("KEYPRESS: DOWN ");
            break;

        case DIR_LEFT:
            printf("KEYPRESS: LEFT ");
            break;

        case DIR_RIGHT:
            printf("KEYPRESS: RIGHT ");
            break;
    }

    if (_movement_started)
        printf("(pressed)\n");
    else
        printf("(released)\n");
}


//////////////////
// Debug output //
//////////////////

void CBluetoothRemote::print_keypress(keypress_t key)
{
    switch(key)
    {
        case keypress_t::KEY_NONE:
            printf("NONE");
            break;

        case keypress_t::KEY_UP:
            printf("KEY_UP");
            break;

        case keypress_t::KEY_DOWN:
            printf("KEY_DOWN");
            break;

        case keypress_t::KEY_LEFT:
            printf("KEY_LEFT");
            break;

        case keypress_t::KEY_RIGHT:
            printf("KEY_RIGHT");
            break;

        case keypress_t::KEY_OK:
            printf("KEY_OK");
            break;

        case keypress_t::KEY_SHUTTER:
            printf("KEY_SHUTTER");
            break;            
    }
}

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
