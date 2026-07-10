#pragma once

#include <stdint.h>

struct PowerInfo
{
    bool batteryConnected = false;
    bool charging = false;
    bool usbConnected = false;

    uint16_t batteryVoltageMv = 0;
    uint16_t usbVoltageMv = 0;
    uint16_t systemVoltageMv = 0;

    uint8_t batteryPercent = 0;
};