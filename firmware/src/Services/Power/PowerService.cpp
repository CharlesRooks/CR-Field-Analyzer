#include "PowerService.h"

#include <Arduino.h>
#include <LilyGo_AMOLED.h>

LilyGo_AMOLED *PowerService::board = nullptr;
PowerInfo PowerService::info;

void PowerService::Begin(LilyGo_AMOLED *device)
{
    board = device;

    if (board == nullptr)
    {
        return;
    }

    // Required before reading voltage values from the
    // BQ25896 power-management chip on the 1.91-inch AMOLED Plus.
    board->BQ.enableMeasure();

    Update();
}

void PowerService::Update()
{
    static uint32_t lastUpdateMs = 0;

    if (millis() - lastUpdateMs < 1000)
    {
        return;
    }

    lastUpdateMs = millis();

    if (board == nullptr)
    {
        info = PowerInfo{};
        return;
    }

    info.usbConnected = board->BQ.isVbusIn();
    info.charging = board->BQ.isCharging();

    info.batteryVoltageMv = board->BQ.getBattVoltage();
    info.usbVoltageMv = board->BQ.getVbusVoltage();
    info.systemVoltageMv = board->BQ.getSystemVoltage();

    // The BQ charger cannot directly confirm that a battery exists.
    // Infer presence from a plausible single-cell battery voltage.
    info.batteryConnected =
        info.batteryVoltageMv >= 2500 &&
        info.batteryVoltageMv <= 4600;
}

const PowerInfo &PowerService::GetInfo()
{
    return info;
}

bool PowerService::IsBatteryConnected()
{
    return info.batteryConnected;
}

bool PowerService::IsCharging()
{
    return info.charging;
}

bool PowerService::IsUSBConnected()
{
    return info.usbConnected;
}

uint16_t PowerService::GetBatteryVoltageMv()
{
    return info.batteryVoltageMv;
}

uint16_t PowerService::GetUSBVoltageMv()
{
    return info.usbVoltageMv;
}

uint16_t PowerService::GetSystemVoltageMv()
{
    return info.systemVoltageMv;
}

uint8_t PowerService::GetBatteryPercent()
{
    return info.batteryPercent;
}