#ifndef _CMENUAPMODE_H
#define _CMENUAPMODE_H

#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../RemoteAccess/CWifi.h"
#include "../../../RemoteAccess/setupwebinterface.h"
#include "../../../RemoteAccess/QR-Code-generator/c/qrcodegen.h"
#include "../../../CSavedSettings.h"
#include "../../../CAnalogueCapture.h"

class CMenuApMode : public CMenu
{
    public:
        CMenuApMode(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CWifi *wifi, CAnalogueCapture *analogueCapture);
        ~CMenuApMode();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

        void wifi_scan();

    private:
        enum state_t
        {
            INIT             = 0,
            WIFI_SCAN        = 1, // Scan in progress
            WIFI_SCAN_WAIT   = 2, // Delay between scans
            AP_MODE_STARTED  = 3
        };

        enum display_mode_t
        {
            QR_CODE          = 1, // Generated QR code with ssid / password
            SSID_PASS        = 2  // On screen SSID / password
        };

        void show_qr_code(std::string str);
        void set_button_text();
        void get_strongest_networks(std::map<std::string, WlanDetails> *input, std::list<std::string> *output, uint8_t count);

        struct network_t
        {
            std::string name;
            int16_t rssi;
        };

        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CAnalogueCapture *_analogueCapture;
        uint8_t _selected_item;
        CWifi *_wifi;
        state_t _state = state_t::INIT;      
        uint64_t _last_scan_finish_us;  
        SetupWebInterface *_setupwebinterface = NULL;
        std::string _qr_code;
        display_mode_t _display_mode = display_mode_t::QR_CODE;
        display_area _disp_area;
        uint8_t _qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
        bool _qr_code_generated = false;
        uint8_t _wifi_networks_found_count = 0;
        std::list<std::string> _strongest_networks;
        const uint8_t StrongestNetworkDisplayCount = 3;
};



#endif
