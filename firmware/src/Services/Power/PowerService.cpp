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

    board->BQ.enableMeasure();

    Update();
}

void PowerService::Update()
{
    static uint32_t lastUpdateMs = 0;
    const uint32_t now = millis();

    if (now - lastUpdateMs < 1000)
    {
        return;
    }

    lastUpdateMs = now;

    if (board == nullptr)
    {
        info = PowerInfo{};
        return;
    }

    info.batteryVoltageMv = board->BQ.getBattVoltage();
    info.usbVoltageMv = board->BQ.getVbusVoltage();
    info.systemVoltageMv = board->BQ.getSystemVoltage();

    info.usbConnected = info.usbVoltageMv >= 4000;

    info.charging =
        info.usbConnected &&
        board->BQ.isCharging();

    info.batteryConnected =
        info.batteryVoltageMv >= 2500 &&
        info.batteryVoltageMv <= 4600;

    if (info.batteryConnected)
    {
        info.batteryPercent =
            EstimateBatteryPercent(info.batteryVoltageMv);
    }
    else
    {
        info.batteryPercent = 0;
    }
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

void PowerService::FormatStatus(char *buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    if (IsBatteryConnected())
    {
        if (IsCharging())
        {
            snprintf(
                buffer,
                bufferSize,
                "CHG %u%%",
                GetBatteryPercent()
            );
        }
        else if (IsUSBConnected())
        {
            snprintf(
                buffer,
                bufferSize,
                "USB %u%%",
                GetBatteryPercent()
            );
        }
        else
        {
            snprintf(
                buffer,
                bufferSize,
                "BAT %u%%",
                GetBatteryPercent()
            );
        }
    }
    else if (IsUSBConnected())
    {
        snprintf(
            buffer,
            bufferSize,
            "USB"
        );
    }
    else
    {
        snprintf(
            buffer,
            bufferSize,
            "--"
        );
    }
}

uint8_t PowerService::EstimateBatteryPercent(uint16_t voltageMv)
{
    constexpr uint16_t BATTERY_EMPTY_MV = 3300;
    constexpr uint16_t BATTERY_FULL_MV = 4200;

    if (voltageMv <= BATTERY_EMPTY_MV)
    {
        return 0;
    }

    if (voltageMv >= BATTERY_FULL_MV)
    {
        return 100;
    }

    return static_cast<uint8_t>(
        ((voltageMv - BATTERY_EMPTY_MV) * 100UL) /
        (BATTERY_FULL_MV - BATTERY_EMPTY_MV)
    );
}