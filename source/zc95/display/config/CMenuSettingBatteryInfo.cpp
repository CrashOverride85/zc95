/*
* ZC95
* Copyright (C) 2025  CrashOverride85
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

#include "CMenuSettingBatteryInfo.h"
#include "CMenuSettingsBatteryReset.h"
#include "../git_version.h"

CMenuSettingBatteryInfo::CMenuSettingBatteryInfo(CDisplay* display, IHal* hal)
{
    printf("CMenuSettingBatteryInfo() \n");
    _display = display;
    _hal = hal;
    _allow_batt_reset = (_hal->hardware_version() == zc95_version_t::MKII);
}

CMenuSettingBatteryInfo::~CMenuSettingBatteryInfo()
{
    printf("~CMenuSettingBatteryInfo() \n");
}

void CMenuSettingBatteryInfo::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A:
                if (_hal->hardware_variant() == hw_variant_t::V2_2)
                {
                    if (_page == page_t::PAGE_1)
                        _page = page_t::PAGE_2;
                    else
                        _page = page_t::PAGE_1;
                
                    update_menu_text();
                }
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // Batt reset
                if (_allow_batt_reset)
                {
                    set_active_menu(new CMenuSettingsBatteryReset(_display, _hal));
                }
                break;

            default:
                break;
        }
    }
}

void CMenuSettingBatteryInfo::adjust_rotary_encoder_change(int8_t change)
{

}

void CMenuSettingBatteryInfo::draw()
{
    if (_page == page_t::PAGE_1)
        draw_page_1();
    else
        draw_page_2();
}

void CMenuSettingBatteryInfo::draw_page_1()
{
    uint8_t line = 2;
    display_area disp_area = _display->get_display_area();
    int16_t stat = -1;

    IPowerManagement* power_management = _hal->power_management();

    put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF),     "SoC      : " + std::to_string(power_management->get_battery_percentage()) + "%");
    
    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::BatVoltage))
    {
        char buffer[10] = {0};
        float voltage = (float)stat / (float)1000;
        sprintf(buffer, "%.2f V", voltage);
        
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "Voltage  : " + std::string(buffer));
    }

    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::RemainingCapacity))
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "Remain   : " + std::to_string(stat) + " mAh");
    
    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::FullCapacity))
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "Full cap.: " + std::to_string(stat) + " mAh");

    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::BatCurrent))
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "Current  : " + std::to_string(stat) + " mA");

    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::VbusVoltage))
    {
        char buffer[10] = {0};
        float voltage = (float)stat / (float)1000;
        sprintf(buffer, "%.2f V", voltage);
        
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "USB VBUS : " + std::string(buffer));
    }
}

void CMenuSettingBatteryInfo::draw_page_2()
{
    uint8_t line = 2;
    display_area disp_area = _display->get_display_area();

    IPowerManagement* power_management = _hal->power_management();

    std::vector<std::pair<std::string, std::string>> charger_status = power_management->get_charger_status();

    for (const auto& item : charger_status)
    {
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), item.first + " : " + item.second);
    }

    int16_t stat = -1;
    if (power_management->get_stat(&stat, IPowerManagement::power_stat_t::InputCurrentLimit))
        put_text_line(disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF), "Input lim. : " + std::to_string(stat) + "mA");
    
}

void CMenuSettingBatteryInfo::put_text_line(int16_t x, int16_t y, uint8_t line, hagl_color_t colour, std::string text)
{
    _display->put_text(text, x, y + (line * 10), colour);
}

void CMenuSettingBatteryInfo::show()
{
    update_menu_text();

    _exit_menu = false;
}

void CMenuSettingBatteryInfo::update_menu_text()
{
    if (_page == page_t::PAGE_1)
    {
        if (_hal->hardware_variant() == hw_variant_t::V2_2)
            _display->set_option_a("Charger sts.");
        else
            _display->set_option_a("");

        _display->set_option_b("Back");

        if (_allow_batt_reset)
            _display->set_option_c("Batt Reset");
        else
            _display->set_option_c("");

        _display->set_option_d("");
    }

    else // page 2
    {
        _display->set_option_a("Batt info");
        _display->set_option_b("Back");

        _display->set_option_c("");
        _display->set_option_d("");
    }
}