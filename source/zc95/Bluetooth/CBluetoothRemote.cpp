#include "CBluetoothRemote.h"

CBluetoothRemote::CBluetoothRemote()
{
    printf("CBluetoothRemote()\n");
    _pressed = false;
}

CBluetoothRemote::~CBluetoothRemote()
{
    printf("~CBluetoothRemote()\n");
}


void CBluetoothRemote::process_input(uint16_t usage_page, uint16_t usage, int32_t value)
{
    switch(usage_page)
    {
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            process_desktop_page(usage, value);
            break;

        case HID_USAGE_PAGE_BUTTON:
            process_button_page(usage, value);
            break;

        default:
            break;
    }

}

void CBluetoothRemote::process_desktop_page(uint16_t usage, int32_t value)
{
    if (usage == HID_USAGE_GENERIC_DESKTOP_X)
    {
        _current_x = value;
    }
    else if (usage == HID_USAGE_GENERIC_DESKTOP_Y)
    {
        _current_y = value;
    }
}

void CBluetoothRemote::process_button_page(uint16_t usage, int32_t value)
{
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
}

void CBluetoothRemote::end_of_input()
{
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

    if (_pressed)
        printf("(pressed)\n");
    else
        printf("(released)\n");
}
