#pragma once

#include <stdint.h>

enum class WakeReason : uint8_t
{
    Unknown,
    PowerOn,
    Reset,
    BootButton,
    USB
};

class SleepService
{
public:
    static void Begin();

    static WakeReason GetWakeReason();
    static const char *GetWakeReasonText();

    static void EnterDeepSleep();

private:
    static WakeReason wakeReason;

    static void DetectWakeReason();
    static void ConfigureWakeSources();
};