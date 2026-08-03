#pragma once

#include <stdint.h>

class LilyGo_AMOLED;

class DisplayService
{
public:
    static void Begin(LilyGo_AMOLED *device);

    static void SetBrightness(uint8_t value);
    static uint8_t GetBrightness();

    static void TurnOn();
    static void TurnOff();
    static bool IsOn();

    static void Dim();
    static void RestoreBrightness();

private:
    static LilyGo_AMOLED *board;

    static bool displayOn;
    static bool brightnessDimmed;

    static uint8_t currentBrightness;
    static uint8_t savedBrightness;
};