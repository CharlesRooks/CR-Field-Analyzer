#pragma once

#include <Arduino.h>

class SystemService
{
public:
    static uint32_t GetFlashSizeMB();
    static bool HasPSRAM();
    static uint32_t GetFreeHeapKB();
    static uint32_t GetUptimeSeconds();
    static String GetFormattedUptime();
    static String GetChipModel();
    static uint32_t GetCPUFrequencyMHz();
};