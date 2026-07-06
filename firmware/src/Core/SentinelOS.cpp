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
    delay(1000);

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

    SplashScreen::Show();
    delay(2500);

    dashboard.Show();

    Serial.println("Dashboard Loaded.");
}

void SentinelOS::Update()
{
    dashboard.Update();
    lv_timer_handler();
    delay(5);
}