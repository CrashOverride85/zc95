#include "CWifi.h"

#include "pico/cyw43_arch.h"
#include <stdio.h>
#include "pico/stdlib.h"

CWifi::CWifi()
{
    if (cyw43_arch_init())
    {
        printf("Failed to init wifi/cyw43\n");
        _init_success = false;
    }
    else
    {
        printf("wifi/cyw43 init success\n");
        _init_success = true;
    }
}

void CWifi::loop()
{
    if (!_init_success)
        return;
    
    cyw43_arch_poll();
}
