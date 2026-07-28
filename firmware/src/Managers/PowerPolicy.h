#pragma once

#include <stdint.h>

struct PowerPolicy
{
    // Master enable for automatic power management.
    bool automaticPowerSavingEnabled = true;

    // Allow automatic sleep while external USB power is connected.
    bool allowAutomaticSleepOnUsb = false;

    // Allow the display to dim automatically.
    bool allowDisplayDimming = true;

    // Allow the display to turn off automatically.
    bool allowDisplaySleep = true;

    // Allow automatic deep sleep.
    bool allowDeepSleep = true;

    // Timeout before dimming the display.
    uint32_t dimTimeoutMs = 30000;

    // Timeout before turning the display off.
    uint32_t displayOffTimeoutMs = 60000;

    // Timeout before entering deep sleep.
    uint32_t deepSleepTimeoutMs = 120000;
};