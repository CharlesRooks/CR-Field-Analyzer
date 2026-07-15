#pragma once

#include "PowerInfo.h"
#include <stddef.h>

class LilyGo_AMOLED;

class PowerService
{
public:
    static void Begin(LilyGo_AMOLED *device);

    static void Update();

    static const PowerInfo &GetInfo();

    static bool IsBatteryConnected();

    static bool IsCharging();

    static bool IsUSBConnected();

    static uint16_t GetBatteryVoltageMv();

    static uint16_t GetUSBVoltageMv();

    static uint16_t GetSystemVoltageMv();

    static uint8_t GetBatteryPercent();

    static void FormatStatus(char *buffer, size_t bufferSize);

    static bool IsLowBattery();
    
    static bool IsCriticalBattery();
    

private:
    static LilyGo_AMOLED *board;
    static PowerInfo info;

    static uint8_t EstimateBatteryPercent(uint16_t voltageMv);
};