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
    uint8_t batteryPercent = 0;
    uint8_t lastDischargePercent = 0;

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

    info.batteryConnected =
        info.batteryVoltageMv >= 2500 &&
        info.batteryVoltageMv <= 4600;

    // Enable charging only when both USB and a valid battery are present.
    if (info.usbConnected && info.batteryConnected)
    {
        if (!board->BQ.isEnableCharge())
        {
            board->BQ.setChargeTargetVoltage(4208);
            board->BQ.setPrechargeCurr(64);
            board->BQ.setChargerConstantCurr(832);
            board->BQ.enableCharge();
        }
    }
    else if (!info.batteryConnected && board->BQ.isEnableCharge())
    {
        // Avoid enabling the charger when no battery is detected.
        board->BQ.disableCharge();
    }

    const auto chargeState = board->BQ.chargeStatus();

    if (!info.batteryConnected)
    {
        info.batteryPercent = 0;
        info.lastDischargePercent = 0;
    }
    else if (!info.usbConnected)
    {
        const uint8_t estimatedPercent =
            EstimateBatteryPercent(info.batteryVoltageMv);

        if (info.lastDischargePercent == 0)
        {
            info.lastDischargePercent = estimatedPercent;
        }
        else if (estimatedPercent > info.lastDischargePercent + 1)
        {
            info.lastDischargePercent++;
        }
        else if (estimatedPercent + 1 < info.lastDischargePercent)
        {
            info.lastDischargePercent--;
        }

        info.batteryPercent = info.lastDischargePercent;
    }
    else if (chargeState == PowersBQ25896::CHARGE_STATE_DONE)
    {
        info.batteryPercent = 100;
        info.lastDischargePercent = 100;
    }
    else
    {
        // Charging voltage is artificially elevated, so preserve the last
        // battery-only estimate instead of recalculating toward 100%.
        if (info.lastDischargePercent == 0)
        {
            uint8_t startupEstimate =
                EstimateBatteryPercent(info.batteryVoltageMv);

            info.lastDischargePercent =
                startupEstimate > 95 ? 95 : startupEstimate;
        }

        info.batteryPercent = info.lastDischargePercent;
    }

    info.charging =
        info.usbConnected &&
        (
            chargeState == PowersBQ25896::CHARGE_STATE_PRE_CHARGE ||
            chargeState == PowersBQ25896::CHARGE_STATE_FAST_CHARGE
        );

    if (!info.batteryConnected)
    {
        info.batteryPercent = 0;
        info.lastDischargePercent = 0;
    }
    else if (!info.usbConnected)
    {
        // Battery voltage is most useful for estimation when not charging.
        info.batteryPercent =
            EstimateBatteryPercent(info.batteryVoltageMv);

        info.lastDischargePercent = info.batteryPercent;
    }
    else if (chargeState == PowersBQ25896::CHARGE_STATE_DONE)
    {
        info.batteryPercent = 100;
        info.lastDischargePercent = 100;
    }
    else
    {
        // Charging voltage is elevated and would produce a falsely high
        // percentage. Keep the last battery-only estimate instead.
        info.batteryPercent = info.lastDischargePercent;
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