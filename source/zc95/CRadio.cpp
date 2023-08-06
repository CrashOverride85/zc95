#include "CRadio.h"

CRadio::CRadio()
{
}

CRadio::~CRadio()
{
}

bool CRadio::bluetooth(bool required)
{
    if (_bluetooth != required)
    {
        _bluetooth = required;
        set_radio();
    }

    return _radio_active;
}

bool CRadio::wifi(bool required)
{
    if (_wifi != required)
    {
        _wifi = required;
        set_radio();
    }

    return _radio_active;
}

void CRadio::set_radio()
{
    //printf("CRadio::set_radio(), _bluetooth=%d, _wifi=%d, _radio_active=%d\n", _bluetooth, _wifi, _radio_active);
    // init radio if required
    if ((_bluetooth || _wifi) && !_radio_active)
    {
        if (cyw43_arch_init())
        {
            printf("CRadio::set_radio(): Failed to init cyw43\n");
        }
        else
        {
            printf("CRadio::set_radio(): cyw43 init success\n");
            _radio_active = true;
        }
    }

    // deinit radio if no longer required
    if (_radio_active && !_bluetooth && !_wifi)
    {
        printf("CRadio::set_radio(): Deinit cyw43\n");
        cyw43_arch_deinit();
        _radio_active = false;
    }
}

void CRadio::loop()
{
    if (_radio_active)
        cyw43_arch_poll();
}