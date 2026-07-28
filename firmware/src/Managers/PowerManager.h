#pragma once

#include "PowerPolicy.h"
#include <stdint.h>

class PowerManager
{
public:
    static void Begin();
    static void Update();
    static const PowerPolicy &GetActivePolicy();
    static void NotifyActivity();
    static uint32_t GetIdleTimeMs();

private:
    static PowerPolicy usbPolicy;
    static PowerPolicy batteryPolicy;
    static const PowerPolicy *activePolicy;
    static void SelectActivePolicy();
    static uint32_t lastActivityMs;
    static bool displayDimmed;
    static void ApplyDisplayPolicy();
    static void RestoreDisplayBrightness();
};