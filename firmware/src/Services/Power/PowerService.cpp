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

    info.wasUsbConnected = info.usbConnected;

    info.usbConnected = info.usbVoltageMv >= 4000;

    info.batteryConnected =
        info.batteryVoltageMv >= 2500 &&
        info.batteryVoltageMv <= 4600;

    // Enable charging only when USB and a valid battery are present.
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
        board->BQ.disableCharge();
    }

   const auto chargeState = board->BQ.chargeStatus();

    info.charging =
        info.usbConnected &&
        (
            chargeState == PowersBQ25896::CHARGE_STATE_PRE_CHARGE ||
            chargeState == PowersBQ25896::CHARGE_STATE_FAST_CHARGE
        );

    info.chargeComplete =
        info.usbConnected &&
        chargeState == PowersBQ25896::CHARGE_STATE_DONE;

    if (!info.batteryConnected)
    {
        info.batteryPercent = 0;
        info.lastDischargePercent = 0;
    }
    else if (!info.usbConnected)
    {
        const uint8_t estimatedPercent =
            EstimateBatteryPercent(info.batteryVoltageMv);

        if (info.wasUsbConnected)
        {
            // USB was just removed. Keep the last trusted battery-only percentage.
            // The battery voltage is temporarily elevated immediately after charging.
            if (info.lastDischargePercent == 0)
            {
                info.lastDischargePercent = estimatedPercent;
            }
        }
        else if (info.lastDischargePercent == 0)
        {
            info.lastDischargePercent = estimatedPercent;
        }
        else if (estimatedPercent < info.lastDischargePercent)
        {
            // During normal battery operation, decrease gradually and never rebound.
            info.lastDischargePercent--;
        }

        info.batteryPercent = info.lastDischargePercent;
    }
    else if (chargeState == PowersBQ25896::CHARGE_STATE_DONE)
    {
        // Display full charge while USB is connected, but preserve the last
        // battery-only estimate for when external power is removed.
        info.batteryPercent = 100;
    }
    else
    {
        // Preserve the last battery-only estimate while charging.
        if (info.lastDischargePercent == 0)
        {
            uint8_t startupEstimate =
                EstimateBatteryPercent(info.batteryVoltageMv);

            info.lastDischargePercent =
                startupEstimate > 95 ? 95 : startupEstimate;
        }

        info.batteryPercent = info.lastDischargePercent;
    }

    info.lowBattery =
        info.batteryConnected &&
        !info.usbConnected &&
        info.batteryPercent <= 20;

    info.criticalBattery =
        info.batteryConnected &&
        !info.usbConnected &&
        info.batteryPercent <= 10;
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

bool PowerService::IsLowBattery()
{
    return info.lowBattery;
}

bool PowerService::IsCriticalBattery()
{
    return info.criticalBattery;
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
        if (info.chargeComplete)
        {
            snprintf(
                buffer,
                bufferSize,
                "FULL 100%%"
            );
        }
        else if (IsCharging())
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
        else if (IsCriticalBattery())
        {
            snprintf(
                buffer,
                bufferSize,
                "CRIT %u%%",
                GetBatteryPercent()
            );
        }
        else if (IsLowBattery())
        {
            snprintf(
                buffer,
                bufferSize,
                "LOW %u%%",
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
    struct VoltagePoint
    {
        uint16_t voltageMv;
        uint8_t percent;
    };

    static constexpr VoltagePoint curve[] =
    {
        {4200, 100},
        {4150, 95},
        {4110, 90},
        {4080, 85},
        {4020, 80},
        {3980, 75},
        {3950, 70},
        {3910, 65},
        {3870, 60},
        {3850, 55},
        {3820, 50},
        {3800, 45},
        {3780, 40},
        {3760, 35},
        {3740, 30},
        {3710, 25},
        {3680, 20},
        {3650, 15},
        {3600, 10},
        {3500, 5},
        {3300, 0}
    };

    constexpr size_t pointCount =
        sizeof(curve) / sizeof(curve[0]);

    if (voltageMv >= curve[0].voltageMv)
    {
        return curve[0].percent;
    }

    if (voltageMv <= curve[pointCount - 1].voltageMv)
    {
        return curve[pointCount - 1].percent;
    }

    for (size_t i = 0; i < pointCount - 1; i++)
    {
        const VoltagePoint &upper = curve[i];
        const VoltagePoint &lower = curve[i + 1];

        if (voltageMv <= upper.voltageMv &&
            voltageMv >= lower.voltageMv)
        {
            const uint16_t voltageRange =
                upper.voltageMv - lower.voltageMv;

            const uint16_t voltageOffset =
                voltageMv - lower.voltageMv;

            const uint8_t percentRange =
                upper.percent - lower.percent;

            return lower.percent +
                   static_cast<uint8_t>(
                       (voltageOffset * percentRange) /
                       voltageRange
                   );
        }
    }

    return 0;
}