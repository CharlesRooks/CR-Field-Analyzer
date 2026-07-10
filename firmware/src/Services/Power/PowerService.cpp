#include "PowerService.h"

LilyGo_AMOLED *PowerService::board = nullptr;
PowerInfo PowerService::info;

void PowerService::Begin(LilyGo_AMOLED *device)
{
    board = device;
}

void PowerService::Update()
{
    // PMU integration will be added in Milestone 7.4
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