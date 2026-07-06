#include "SystemService.h"

uint32_t SystemService::GetFlashSizeMB()
{
    return ESP.getFlashChipSize() / (1024 * 1024);
}

bool SystemService::HasPSRAM()
{
    return psramFound();
}

uint32_t SystemService::GetFreeHeapKB()
{
    return ESP.getFreeHeap() / 1024;
}

uint32_t SystemService::GetUptimeSeconds()
{
    return millis() / 1000;
}

String SystemService::GetFormattedUptime()
{
    uint32_t totalSeconds = millis() / 1000;

    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;

    char buffer[16];

    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);

    return String(buffer);
}

String SystemService::GetChipModel()
{
    return ESP.getChipModel();
}

uint32_t SystemService::GetCPUFrequencyMHz()
{
    return ESP.getCpuFreqMHz();
}