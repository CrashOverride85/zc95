/*
 * Author: Floris Bos
 * License: public domain / unlicense
 */

#include "wlanscanner.h"
#include "pico/stdlib.h"

WlanScanner *WlanScanner::_wlanscanner = NULL;

WlanScanner::WlanScanner()
{

}

WlanScanner *WlanScanner::instance()
{
    if (!_wlanscanner)
        _wlanscanner = new WlanScanner();
    
    return _wlanscanner;
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result)
{
    static_cast<WlanScanner *>(env)->addScanResult(result);
    return 0;
}

void WlanScanner::startScanning()
{
    cyw43_wifi_scan_options_t scan_options = {0};    
    cyw43_wifi_scan(&cyw43_state, &scan_options, this, scan_result);
}

void WlanScanner::addScanResult(const cyw43_ev_scan_result_t *result)
{
    if (result && result->ssid && result->ssid[0])
    {
        std::string ssid = (char *) result->ssid;
        WlanDetails details;

        details.rssi = result->rssi;
        details.needsUsername = false;
        details.needsPassword = result->auth_mode != CYW43_AUTH_OPEN;
        _ssids[ssid] = details;
    }
}

std::map<std::string, WlanDetails> *WlanScanner::getSSIDs()
{
    return &_ssids;
}

