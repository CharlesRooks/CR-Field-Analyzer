#include "SentinelOS.h"

#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <esp_system.h>
#include "Messaging/MessageBus.h"
#include <USB.h>

#include "../Screens/SplashScreen.h"
#include "../Services/Power/PowerService.h"
#include "../Services/Power/SleepService.h"
#include "../Services/Display/DisplayService.h"
#include "../Managers/PowerManager.h"
#include "../Services/WiFi/WiFiService.h"
#include "../Services/Storage/StorageService.h"
#include "../Services/Time/TimeService.h"
#include "../Services/USB/UsbStorageService.h"
#include "../Services/Survey/SiteSurveyService.h"
#include "../Managers/SiteSurveyManager.h"


static LilyGo_Class amoled;

SentinelOS *SentinelOS::instance = nullptr;

static void PublishUserActivity()
{
    Message message{};
    message.type = MessageType::UserActivity;
    message.timestampMs = millis();

    MessageBus::Publish(message);
}

void SentinelOS::Begin()
{
    MessageBus::Begin();

    Serial.begin(115200);

    USB.begin();

    delay(3000);

    instance = this;

    if (!MessageBus::Subscribe(
            MessageType::NavigationChanged,
            SentinelOS::HandleMessage))
    {
        Serial.println(
            "ERROR: SentinelOS failed to subscribe to NavigationChanged");
    }

    if (!MessageBus::Subscribe(
            MessageType::WiFiMeasurementSessionCompleted,
            SentinelOS::HandleMessage))
    {
        Serial.println(
            "ERROR: SentinelOS failed to subscribe to "
            "WiFiMeasurementSessionCompleted");
    }

    Serial.println("BOOT CHECK");

    Serial.println();
    Serial.println("Starting SentinelOS...");

    pinMode(0, INPUT_PULLUP);

    Serial.printf(
        "Reset reason: %d\n",
        static_cast<int>(esp_reset_reason())
    );

    if (!amoled.begin())
    {
        Serial.println("ERROR: AMOLED initialization failed!");

        while (true)
        {
            delay(1000);
        }
    }

    TimeService::Begin();
    SiteSurveyService::Begin();
    StorageService::Begin();
    UsbStorageService::Begin();

    beginLvglHelper(amoled);

    input.Begin(&amoled);
    PowerService::Begin(&amoled);
    PowerService::Update();

    PowerManager::Begin();

    SleepService::Begin();

    DisplayService::Begin(&amoled);

    WiFiService::Begin();
    SiteSurveyManager::Begin();

    Serial.printf(
        "Wake reason: %s\n",
        SleepService::GetWakeReasonText()
    );

    ChangeState(AppState::Splash);

}

void SentinelOS::Update()
{
    switch (currentState)
    {
        case AppState::Splash:
        {
            if (millis() - stateStartMs >= 2500)
            {
                ChangeState(AppState::Running);
            }
            break;
        }

        case AppState::Running:
        {
            input.Update();

            
            static uint32_t bootPressedMs = 0;
            static bool sleepTriggered = false;
            static bool previousBootPressed = false;

            const bool bootPressed = digitalRead(0) == LOW;

            if (bootPressed && !previousBootPressed)
            {
                PublishUserActivity();
            }

            previousBootPressed = bootPressed;

            if (bootPressed)
            {
                if (bootPressedMs == 0)
                {
                    bootPressedMs = millis();
                }
                else if (!sleepTriggered &&
                        millis() - bootPressedMs >= 2000)
                {
                    sleepTriggered = true;
                    SleepService::EnterDeepSleep();
                }
            }
            else
            {
                bootPressedMs = 0;
                sleepTriggered = false;
            }

            PowerService::Update();
            UsbStorageService::Update();
            WiFiService::Update();

            if (measurementSavePending)
            {
                const WiFiMeasurementSummary &summary =
                    WiFiService::GetMeasurementSummary();

                char capturedLocal[32] = {};

                const bool capturedTimeValid =
                    pendingMeasurementCapturedEpoch != 0 &&
                    TimeService::FormatEpochIsoLocal(
                        pendingMeasurementCapturedEpoch,
                        capturedLocal,
                        sizeof(capturedLocal));

                if (!StorageService::SaveMeasurementSummary(
                        summary,
                        pendingMeasurementCompletedAtMs,
                        capturedTimeValid
                            ? pendingMeasurementCapturedEpoch
                            : 0,
                        capturedTimeValid
                            ? capturedLocal
                            : nullptr))
                {
                    Serial.println(
                        "SentinelOS: Completed measurement "
                        "could not be persisted");
                }

                measurementSavePending = false;
                pendingMeasurementCompletedAtMs = 0;
                pendingMeasurementCapturedEpoch = 0;
            }

            PowerManager::Update();
            frame.Update();
            navigation.Update();

            break;
        }

        case AppState::Boot:
        default:
            break;
    }

    lv_timer_handler();
    delay(5);

}

void SentinelOS::ChangeState(AppState newState)
{
    currentState = newState;
    stateStartMs = millis();

    switch (currentState)
    {
        case AppState::Splash:
            SplashScreen::Show();
            Serial.println("State: Splash");
            break;

        case AppState::Running:
            frame.Show(ScreenID::Dashboard);
            navigation.Begin(frame.GetContentArea());
            navigation.Show(ScreenID::Dashboard);

            WiFiService::StartScan();

            Serial.println("State: Running");
            break;

        case AppState::Boot:
        default:
            break;
    }
}

void SentinelOS::HandleMessage(const Message &message)
{
    if (instance == nullptr)
    {
        return;
    }

    switch (message.type)
    {
        case MessageType::NavigationChanged:
            instance->frame.SetCurrent(
                message.screenId);
            break;

        case MessageType::WiFiMeasurementSessionCompleted:
            // Keep the synchronous MessageBus handler short.
            // The SD write is deferred to the main update loop, while
            // the wall-clock timestamp is captured at completion.
            instance->measurementSavePending = true;
            instance->pendingMeasurementCompletedAtMs =
                message.timestampMs;

            if (!TimeService::GetEpochTime(
                    instance->
                        pendingMeasurementCapturedEpoch))
            {
                instance->
                    pendingMeasurementCapturedEpoch = 0;
            }
            break;

        default:
            break;
    }
}
