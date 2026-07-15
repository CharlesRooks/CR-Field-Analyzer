#include "./SleepService.h"

#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_system.h>

WakeReason SleepService::wakeReason = WakeReason::Unknown;

void SleepService::Begin()
{
    DetectWakeReason();
}

WakeReason SleepService::GetWakeReason()
{
    return wakeReason;
}

const char *SleepService::GetWakeReasonText()
{
    switch (wakeReason)
    {
        case WakeReason::PowerOn:
            return "Power On";

        case WakeReason::Reset:
            return "Reset";

        case WakeReason::BootButton:
            return "BOOT Button";

        case WakeReason::USB:
            return "USB";

        case WakeReason::Unknown:
        default:
            return "Unknown";
    }
}

void SleepService::EnterDeepSleep()
{
    ConfigureWakeSources();

    Serial.println("Release BOOT button to enter deep sleep...");
    Serial.flush();

    // BOOT is active-low. Do not enter sleep while the wake condition
    // is already active, or the ESP32-S3 will wake immediately.
    while (digitalRead(0) == LOW)
    {
        delay(10);
    }

    delay(100);

    Serial.println("Entering deep sleep...");
    Serial.flush();

    esp_deep_sleep_start();
}

void SleepService::DetectWakeReason()
{
    const esp_sleep_wakeup_cause_t wakeCause =
        esp_sleep_get_wakeup_cause();

    switch (wakeCause)
    {
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:
            wakeReason = WakeReason::BootButton;
            break;

        default:
        {
            const esp_reset_reason_t resetReason =
                esp_reset_reason();

            if (resetReason == ESP_RST_POWERON)
            {
                wakeReason = WakeReason::PowerOn;
            }
            else
            {
                wakeReason = WakeReason::Reset;
            }

            break;
        }
    }
}

void SleepService::ConfigureWakeSources()
{
    // Wake when the BOOT button pulls GPIO0 low.
    esp_sleep_enable_ext0_wakeup(
        GPIO_NUM_0,
        0
    );
}