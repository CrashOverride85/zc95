#include "CMenuApMode.h"
#include "pico/cyw43_arch.h"
#include "../RemoteAccess/setupwebinterface.h"

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
    _disp_area = _display->get_display_area();
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

    _wifi->stop();
    WlanScanner::instance()->reset();
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
            case Button::A: // qr / text toggle
                if (_state == state_t::AP_MODE_STARTED)
                {
                    if (_display_mode == display_mode_t::QR_CODE)
                        _display_mode =  display_mode_t::SSID_PASS;
                    else
                        _display_mode =  display_mode_t::QR_CODE;
                    
                    set_button_text();
                }
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C:
                if (_state != state_t::AP_MODE_STARTED && _wifi_networks_found_count > 0)
                {
                    printf("CMenuApMode::button_pressed(): start AP mode\n");
                    _setupwebinterface->startAccessPoint();
                    _state = state_t::AP_MODE_STARTED;
                    set_button_text();
                    _analogueCapture->start();
                    _qr_code = _setupwebinterface->getQrCode();
                }
                break;

            case Button::D:
                break;
        }
    }
}

void CMenuApMode::set_button_text()
{
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");

    if (_state != state_t::AP_MODE_STARTED)
    {
        _display->set_option_a("");

        if (_wifi_networks_found_count > 0)
        {
            _display->set_option_c("Start AP");
        }
    }
    else
    {
        if (_display_mode == display_mode_t::QR_CODE)
        {
            _display->set_option_a("SSID/PASS");
        }
        else
        {
            _display->set_option_a("QR Code");
        }
    }
}

void CMenuApMode::adjust_rotary_encoder_change(int8_t change)
{

}

void CMenuApMode::draw()
{
    wifi_scan();
    int y = 0;
    
    if (_state != state_t::AP_MODE_STARTED)
    {
        _display->put_text("Scanning... ", _disp_area.x0, _disp_area.y0+20, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

        _display->put_text("Strongest " + std::to_string(StrongestNetworkDisplayCount) + " wlans:", _disp_area.x0, _disp_area.y0+40, hagl_color(_display->get_hagl_backed(), 0x50, 0x50, 0x50));
        std::list<std::string> ::iterator it_networks;
        uint8_t y = 10;
        for (it_networks = _strongest_networks.begin(); it_networks != _strongest_networks.end(); it_networks++)
        {
            _display->put_text(*it_networks, _disp_area.x0, _disp_area.y0+((_disp_area.y1-_disp_area.y0)/2) + y, hagl_color(_display->get_hagl_backed(), 0x50, 0x50, 0x50));
            y += 10;
        }

    }
    else if (_state == state_t::AP_MODE_STARTED)
    {
        switch (_display_mode)
        {       
            case display_mode_t::QR_CODE:
                show_qr_code(_qr_code);
                break;

            case display_mode_t::SSID_PASS:
                _display->put_text("SSID: ", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
                y+=10;
                _display->put_text(_setupwebinterface->getApSsid(), _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
                y+=20;

                _display->put_text("PASS: ", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
                y+=10;
                _display->put_text(_setupwebinterface->getApPsk() , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
                y+=20;

                _display->put_text("Setup URL: ", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
                y+=10;
                _display->put_text("http://192.168.4.1/", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));

                break;
        }
    }
}

void CMenuApMode::show()
{
    set_button_text();
}

void CMenuApMode::wifi_scan()
{
    if (_state == state_t::INIT)
    {
         // Something (DMA?) about the analogue capture breaks wifi scanning, and causes a crash.
         // Get "[CYW43] core not up" and "[CYW43] HT not ready" errors without stopping it.
        _analogueCapture->stop();

        printf("CMenuApMode::wifi_scan(): Start scanning for wifi networks\n");
        _wifi->start_ap();
        WlanScanner::instance()->startScanning();
        _state = state_t::WIFI_SCAN;
        set_button_text();
    }

    else if (_state == state_t::WIFI_SCAN)
    {
        if (!WlanScanner::instance()->isScanInProgress())
        {
            printf("CMenuApMode::wifi_scan(): scan completed\n");
            _state = state_t::WIFI_SCAN_WAIT;
            _last_scan_finish_us = time_us_64();

            std::map<std::string, WlanDetails> *ssids = WlanScanner::instance()->getSSIDs();
            
            _wifi_networks_found_count = ssids->size();
            get_strongest_networks(ssids, &_strongest_networks, StrongestNetworkDisplayCount);
            set_button_text();
        }
    }

    else if (_state == state_t::WIFI_SCAN_WAIT)
    {
        const int seconds = 2;
        if (time_us_64() > (_last_scan_finish_us + (1000 * 1000 * seconds)))
        {
            printf("CMenuApMode::wifi_scan(): starting next scan\n");
            WlanScanner::instance()->startScanning();
            _state = state_t::WIFI_SCAN;
        }
    }
}

void CMenuApMode::get_strongest_networks(std::map<std::string, WlanDetails> *input, std::list<std::string> *output, uint8_t count)
{
    std::list<network_t> networks;
    std::map<std::string, WlanDetails> ::iterator it_input;

    for (it_input = input->begin(); it_input != input->end(); it_input++)
    {
        network_t network;
        network.name = it_input->first;
        network.rssi = it_input->second.rssi;
        networks.push_back(network);
    }

    networks.sort([](const network_t & a, const network_t & b) { return a.rssi > b.rssi; });

    std::list<network_t> ::iterator it_networks;

    uint8_t i = 0;
    output->clear();
    for (it_networks = networks.begin(); it_networks != networks.end() && i++ < count; it_networks++)
    {
        output->push_back(it_networks->name);
    }
}

void CMenuApMode::show_qr_code(std::string str)
{
    display_area area = _display->get_display_area();
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];

    if (str == "")
        return;

    // Generating the QR code from a string is slow (ms not us), so only do it once, not every time we redraw the screen
    if (!_qr_code_generated)
    {
        bool ok = qrcodegen_encodeText(str.c_str(), tempBuffer, _qrcode, qrcodegen_Ecc_MEDIUM,
            qrcodegen_VERSION_MIN, 10, qrcodegen_Mask_AUTO, true);

        if (ok)
        {
            _qr_code_generated = true;
        }
        else
        {
            printf("Failed to generate QR code\n");
            return;
        }
    }

    hagl_color_t colour_black = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0x00);
    hagl_color_t colour_white = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF);

    int size = qrcodegen_getSize(_qrcode);

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

            if (qrcodegen_getModule(_qrcode, x, y))
                hagl_fill_rectangle(_display->get_hagl_backed(), disp_x, disp_y, disp_x+blocksize, disp_y+blocksize, colour_white);
            else
                hagl_fill_rectangle(_display->get_hagl_backed(), disp_x, disp_y, disp_x+blocksize, disp_y+blocksize, colour_black);
        }
    }
}
