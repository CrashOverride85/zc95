#ifndef _CMENUSETTINGABOUT_H
#define _CMENUSETTINGABOUT_H

#include "../CMenu.h"
#include "../CDisplay.h"
#include "../HwCheck/CHwCheck.h"
#include "../../Hal/IHal.h"

class CMenuSettingAbout : public CMenu
{
    public:
        CMenuSettingAbout(CDisplay* display, IHal *hal, CHwCheck *hwCheck);
        ~CMenuSettingAbout();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        enum screen_mode_t
        {
            MAIN_FW     = 0,
            BOOTLOAD_FW = 1
        };

        void put_text_line(std::string text, int16_t x, int16_t y, uint8_t line, hagl_color_t colour);
        void set_c_button_text();
        std::string get_zc95_bootloader_version();
        CDisplay* _display;
        IHal *_hal;
        CHwCheck *_hwCheck;
        screen_mode_t _mode;
        std::string _zc624_fw_ver;
        std::string _zc624_bl_fw_ver;
};

#endif
