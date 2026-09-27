#ifndef _CMENUSETTINGBATTERYINFO_H
#define _CMENUSETTINGBATTERYINFO_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "../../Hal/IHal.h"
#include "../../PowerManagement/IPowerManagement.h"

class CMenuSettingBatteryInfo : public CMenu
{
    public:
        CMenuSettingBatteryInfo(CDisplay* display, IHal* hal);
        ~CMenuSettingBatteryInfo();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        enum class page_t
        {
            PAGE_1,
            PAGE_2
        };

        void put_text_line(int16_t x, int16_t y, uint8_t line, hagl_color_t colour, std::string text);
        void draw_page_1();
        void draw_page_2();
        void update_menu_text();
        CDisplay* _display;
        IHal* _hal;
        bool _allow_batt_reset = false;
        page_t _page = page_t::PAGE_1;
};

#endif
