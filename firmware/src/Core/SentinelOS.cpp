#include "SentinelOS.h"

#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>

#include "../Screens/SplashScreen.h"

static LilyGo_Class amoled;

void SentinelOS::Begin()
{
    Serial.begin(115200);
    delay(3000);
    Serial.println("BOOT CHECK");

    Serial.println();
    Serial.println("Starting SentinelOS...");

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

    ChangeState(AppState::Splash);
}

void SentinelOS::Update()
{
    switch (currentState)
    {
        case AppState::Splash:
            if (millis() - stateStartMs >= 2500)
            {
                ChangeState(AppState::Running);
            }
            break;

        case AppState::Running:
        {
            InputEvent event = input.Update();

            if (event == InputEvent::SwipeLeft)
            {
                navigation.Next();
            }
            else if (event == InputEvent::SwipeRight)
            {
                navigation.Previous();
            }

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
            navigation.Show(ScreenID::Dashboard);
            Serial.println("State: Running");
            break;

        case AppState::Boot:
        default:
            break;
    }
}