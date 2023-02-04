/*
 * Author: Floris Bos
 * License: public domain / unlicense
 */

#include "setupwebinterface.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include "lwip/arch.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "httpd.h"
#include <string>
#include "../globals.h"

SetupWebInterface::SetupWebInterface(CSavedSettings *saved_settings)
{
    _saved_settings = saved_settings;
}

SetupWebInterface::~SetupWebInterface()
{
}

bool SetupWebInterface::alreadyConfigured()
{
    return !_ssid.empty();
}

std::string SetupWebInterface::savedSSID()
{
    return _ssid;
}


/* Play captive portal and reply with our own IP-address for any DNS query */
static bool dns_query_proc(const char *name, ip4_addr_t *addr)
{
    static const ip4_addr_t ipaddr = IPADDR4_INIT_BYTES(192, 168, 4, 1);
    printf("DNS query for %s\n", name);
    *addr = ipaddr;
    return true;
}

u16_t wlanscan_ssi_handler(int iIndex, char *buf, int buflen, u16_t current_tag_part, u16_t *next_tag_part)
{
    static std::string json_buf;

    if (json_buf.empty())
    {
        std::map<std::string, WlanDetails> *ssids = WlanScanner::instance()->getSSIDs();
        json_buf = "[\n";
        bool first = true;

        for (const auto& [ssid, details]: (*ssids) )
        {
            if (first)
                first = false;
            else 
                json_buf += ",";

            json_buf += "{\"ssid\":\""+ssid+"\""
                       +",\"rssi\":"+std::to_string(details.rssi)
                       +",\"needsUsername\":"+(details.needsUsername ? "true" : "false")
                       +",\"needsPassword\":"+(details.needsPassword ? "true" : "false")
                       +"}\n";
        }

        json_buf += "]";
    }

    if (current_tag_part+buflen > json_buf.size() )
        buflen = json_buf.size()-current_tag_part;
    
    memcpy(buf, json_buf.c_str() + current_tag_part, buflen);

    if (current_tag_part+buflen < json_buf.size() )
    {
        *next_tag_part = current_tag_part+buflen;
    }
    else
    {
        json_buf.clear();
    }

    return buflen;
}

static const char *wlanscan_set_wifi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    std::string ssid, password;

    for (int i=0; i<iNumParams; i++)
    {
        if (strcmp(pcParam[i], "ssid") == 0)
        {
            ssid = pcValue[i];
        }
        else if (strcmp(pcParam[i], "password") == 0)
        {
            password = pcValue[i];
        }        
    }

    if (!ssid.empty())
    {
        SetupWebInterface::saveSettings(ssid, password);
    }

    return "/empty.html";
}

void SetupWebInterface::startAccessPoint()
{
    char ssid[33];

    static const char * ssi_tags[] = {
        "wlanscan"
    };
    static const tCGI cgi_handlers[] = {
        {"/wifi/setnetwork", wlanscan_set_wifi_handler}
    };

    snprintf(ssid, sizeof(ssid), "zc95-%02x%02x%02x%02x%02x%02x",
        netif_default->hwaddr[0], netif_default->hwaddr[1], netif_default->hwaddr[2],
        netif_default->hwaddr[3], netif_default->hwaddr[4], netif_default->hwaddr[5]);

    cyw43_arch_enable_ap_mode(ssid, getApPsk().c_str(), CYW43_AUTH_WPA2_AES_PSK);
    cyw43_wifi_pm(&cyw43_state, 0xa11140);

    // Start DHCP server
    IP4_ADDR(&_gw, 192, 168, 4, 1);
    IP4_ADDR(&_mask, 255, 255, 255, 0);
    dhcp_server_init(&_dhcp_server, &_gw, &_mask);

    while (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK);

    httpd_init(1);
    http_set_ssi_handler(wlanscan_ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));

    _qr_code = std::string("WIFI:S:")+ssid+";T:WPA;P:"+getApPsk()+";;";
    _ap_ssid = std::string(ssid);
}

std::string SetupWebInterface::getApSsid()
{
    return _ap_ssid;
}

// Get a PSK for AP mode. If already set in EEPROM, use that, otherwise generate 
// and return a new one and save in EEPROM for next time
std::string SetupWebInterface::getApPsk()
{
    std::string saved_psk;
    if (_saved_settings->get_wifi_ap_psk(saved_psk))
    {
        return saved_psk;
    }
    
    // Need to generate and save PSK
    char new_psk[33] = {0};
    snprintf(new_psk, sizeof(new_psk), "%08x", LWIP_RAND());
    printf("New AP PSK generated: [%s]\n", new_psk);
    _saved_settings->set_wifi_ap_psk(new_psk);
    _saved_settings->save();

    return new_psk;
}

static void _reboot()
{
   // Display::instance()->showText("Rebooting...");
    /* Reboot after half a second, so that webserver has time to finish sending response */
    watchdog_reboot(0, SRAM_END, 500);
}

void SetupWebInterface::saveSettings(const std::string &ssid, const std::string &psk)
{
    printf("Saving ssid=[%s], psk=[%s]\n", ssid.c_str(), psk.c_str());
    g_SavedSettings->set_wifi_credentials(ssid, psk);
    g_SavedSettings->save();
  //  _reboot();
}

void SetupWebInterface::eraseSavedWlan()
{
  //  pico_mount(true);
  //  pico_unmount();
   // _reboot();
}

std::string SetupWebInterface::getQrCode()
{
    return _qr_code;
}

