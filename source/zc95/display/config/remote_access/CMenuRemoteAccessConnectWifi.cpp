/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
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

#include "CMenuRemoteAccessConnectWifi.h"
#include "CMenuApMode.h"

CMenuRemoteAccessConnectWifi::CMenuRemoteAccessConnectWifi(
    CDisplay* display,
    CGetButtonState *buttons,
    CSavedSettings *saved_settings,
    CWifi *wifi,
    CRoutineOutput *routine_output)
{
    printf("CMenuRemoteAccessConnectWifi() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _wifi = wifi;
    _disp_area = _display->get_display_area();
    _exit_menu = false;
    _routine_output = routine_output;

    std::string psk;
    if (_saved_settings->get_wifi_credentials(_ssid, psk))
    {
        _wifi->connect_to_wifi(_ssid, psk);

        _state = state_t::CONNECTING;
    }
    else
    {
        _state = state_t::NOT_CONFIGURED;
    }
    _routine_output->enable_remote_power_mode();
}

CMenuRemoteAccessConnectWifi::~CMenuRemoteAccessConnectWifi()
{
    printf("~CMenuRemoteAccessConnectWifi() \n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    _wifi->stop_webserver();
    _wifi->stop();
    _routine_output->disable_remote_power_mode();
}

void CMenuRemoteAccessConnectWifi::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A: // "Select"
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // "Up"
                break;

            case Button::D: // "Down"
                break;

            case Button::ROT:
                break;
        }
    }
}

void CMenuRemoteAccessConnectWifi::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

void CMenuRemoteAccessConnectWifi::draw()
{
    if (_state == state_t::NOT_CONFIGURED)
    {
        int y = ((_disp_area.y1-_disp_area.y0)/2);
        _display->put_text("Not configured" , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    }
    else if (_state == state_t::CONNECTING)
    {
        int y = ((_disp_area.y1-_disp_area.y0)/2) - 30;
        _display->put_text("Connecting to:" , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text(_ssid            , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
        y += 10;

        // Show a "..." animation to show we're (trying!) to do something
        char connecting_dots[] = "......";
        if (time_us_64() > _last_connecting_screen_update_us + 200000) // 200ms
        {
            _connecting_dot_count++;
            if (_connecting_dot_count > 6)
                _connecting_dot_count = 1;

            connecting_dots[_connecting_dot_count] = '\0';
        }
        _display->put_text(connecting_dots, _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
        y += 10;
       
        _display->put_text("Status:" , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10; 
        std::string connection_status;
        if (_wifi->get_connection_status(connection_status))
        {
            _state = state_t::CONNECTED;
            _wifi->start_webserver();
        }
        _display->put_text(connection_status, _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
    } 
    else if (_state == state_t::CONNECTED)
    {
        int y = ((_disp_area.y1-_disp_area.y0)/2) - 20;
        _display->put_text("Connected to:"  , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text(_ssid            , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
        y += 10;

        _display->put_text("Address:"       , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        char *ip_address = ip4addr_ntoa(netif_ip4_addr(netif_list));
        _display->put_text(ip_address, _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));

        // If we've lost the connection, re-display the "Connecting..." screen
        std::string connection_status;
        if (!_wifi->get_connection_status(connection_status))
        {
            _state = state_t::CONNECTING;
        }
    } 
}

void CMenuRemoteAccessConnectWifi::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;
}
