#include "SentinelOS.h"

#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <esp_system.h>

#include "../Screens/SplashScreen.h"
#include "../Services/Power/PowerService.h"
#include "../Services/Power/SleepService.h"
#include "../Services/Display/DisplayService.h"
#include "../Managers/PowerManager.h"


static LilyGo_Class amoled;

void SentinelOS::Begin()
{
    Serial.begin(115200);
    delay(3000);
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

    beginLvglHelper(amoled);

    input.Begin(&amoled);
    PowerService::Begin(&amoled);
    PowerService::Update();

    PowerManager::Begin();

    SleepService::Begin();

    DisplayService::Begin(&amoled);

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
            InputEvent event = input.Update();

            if (event == InputEvent::SwipeLeft)
            {
                PowerManager::NotifyActivity();

                navigation.Next();
                frame.SetCurrent(navigation.Current());
            }
            else if (event == InputEvent::SwipeRight)
            {
                PowerManager::NotifyActivity();

                navigation.Previous();
                frame.SetCurrent(navigation.Current());
            }
            else if (event == InputEvent::Tap)
            {
                PowerManager::NotifyActivity();
            }

            static uint32_t bootPressedMs = 0;
            static bool sleepTriggered = false;
            static bool previousBootPressed = false;

            const bool bootPressed = digitalRead(0) == LOW;

            if (bootPressed && !previousBootPressed)
            {
                PowerManager::NotifyActivity();
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
            frame.SetCurrent(navigation.Current());
            Serial.println("State: Running");
            break;

        case AppState::Boot:
        default:
            break;
    }
}