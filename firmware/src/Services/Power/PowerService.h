#pragma once

#include "PowerInfo.h"

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

private:
    static LilyGo_AMOLED *board;
    static PowerInfo info;

    static uint8_t EstimateBatteryPercent(uint16_t voltageMv);
};