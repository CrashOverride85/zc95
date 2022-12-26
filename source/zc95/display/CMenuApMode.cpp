#include "CMenuApMode.h"
#include "pico/cyw43_arch.h"
#include "../RemoteAccess/setupwebinterface.h"
#include "../RemoteAccess/QR-Code-generator/c/qrcodegen.h"

CMenuApMode::CMenuApMode(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CWifi *wifi, CAnalogueCapture *analogueCapture)
{
    printf("CMenuApMode()\n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _wifi = wifi;
    _analogueCapture = analogueCapture;
    _qr_code = "";
    _setupwebinterface = new SetupWebInterface(saved_settings);
}

CMenuApMode::~CMenuApMode()
{
    printf("~CMenuApMode()\n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_setupwebinterface)
    {
        delete _setupwebinterface;
        _setupwebinterface = NULL;
    }
}

void CMenuApMode::button_pressed(Button button)
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
        }
    }
}

void CMenuApMode::adjust_rotary_encoder_change(int8_t change)
{

}

void CMenuApMode::draw()
{
    wifi_scan();
    
    if (_qr_code != "")
    {
        show_qr_code(_qr_code);
    }
}

void CMenuApMode::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");
}


void CMenuApMode::wifi_scan()
{
    if (_state == state_t::INIT)
    {
         // Something (DMA?) about the analogue capture breaks wifi scanning, and causes a crash.
         // Get "[CYW43] core not up" and "[CYW43] HT not ready" errors without stoping it.
        _analogueCapture->stop();

        printf("CMenuApMode::wifi_scan(): Start scanning for wifi networks\n");
        cyw43_arch_enable_sta_mode();
        cyw43_wifi_pm(&cyw43_state, 0xa11140);        
        WlanScanner::instance()->startScanning();
        _state = state_t::WIFI_SCAN;
        _scan_start_us = time_us_64();
    } 
    else if (_state == state_t::WIFI_SCAN)
    {
        const int seconds = 3;
        if (time_us_64() > (_scan_start_us + (1000 * 1000 * seconds)))
        {
            printf("CMenuApMode::wifi_scan(): scan finished, start AP mode\n");
            _setupwebinterface->startAccessPoint();
            _state = state_t::AP_MODE_STARTED;
            _analogueCapture->start();
            _qr_code = _setupwebinterface->getQrCode();
        }
    }
}

void CMenuApMode::show_qr_code(std::string str)
{
    display_area area = _display->get_display_area();

    int buflen = qrcodegen_BUFFER_LEN_FOR_VERSION(10);
    uint8_t qrcode[buflen];
    uint8_t tempBuffer[buflen];

    bool ok = qrcodegen_encodeText(str.c_str(), tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, 10, qrcodegen_Mask_AUTO, true);

    if (!ok)
    {
        printf("Failed to generate QR code\n");
        return;
    }

    color_t colour_black = hagl_color(0x00, 0x00, 0x00);
    color_t colour_white = hagl_color(0xFF, 0xFF, 0xFF);

    int size = qrcodegen_getSize(qrcode);

    int display_area_height = area.y1 - area.y0;
    int display_area_width  = area.x1 - area.x0;
    int display_size = display_area_height;
    if (display_area_width < display_size)
        display_size = display_area_width;

    int blocksize = (display_size)/size;

    for (int x = 0; x < size; x++)
    {
        for (int y = 0; y < size; y++)
        {
            int disp_x = area.x0 + (x * blocksize);
            int disp_y = area.y0 + (y * blocksize) + 2;

            if (qrcodegen_getModule(qrcode, x, y))
                hagl_fill_rectangle(disp_x, disp_y, disp_x+blocksize, disp_y+blocksize, colour_white);
            else
                hagl_fill_rectangle(disp_x, disp_y, disp_x+blocksize, disp_y+blocksize, colour_black);
        }
    }
}
