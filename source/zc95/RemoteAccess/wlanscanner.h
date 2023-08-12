/*
 * Author: Floris Bos
 * License: public domain / unlicense
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include <string>
#include <map>

class WlanDetails
{
public:    
    int16_t rssi;
    bool needsUsername;
    bool needsPassword;
};


class WlanScanner
{
public:
    static WlanScanner *instance();
    void startScanning();
    std::map<std::string, WlanDetails> *getSSIDs();
    void addScanResult(const cyw43_ev_scan_result_t *result);
    bool isScanInProgress();
    void reset();

protected:
    static WlanScanner *_wlanscanner;
    std::map<std::string, WlanDetails> _ssids;
    WlanScanner();
};
