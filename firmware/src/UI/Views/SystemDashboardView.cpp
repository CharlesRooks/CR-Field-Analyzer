#include "SystemDashboardView.h"
#include "../../Services/System/SystemService.h"
#include "../../Services/Power/PowerService.h"
#include <Arduino.h>

void SystemDashboardView::Create(lv_obj_t *parentObject)
{
    parent = parentObject;

    GridLayout layout(parent, 4, 100, 48, 10);

    coreTile.Create(parent,    "Core",    "OK", 0, 0);
    displayTile.Create(parent, "Display", "OK", 0, 0);
    touchTile.Create(parent,   "Touch",   "OK", 0, 0);
    flashTile.Create(parent,   "Flash",   "--", 0, 0);
    psramTile.Create(parent,   "PSRAM",   "--", 0, 0);
    heapTile.Create(parent,    "Heap",    "--", 0, 0);
    uptimeTile.Create(parent,  "Uptime",  "--", 0, 0);
    powerTile.Create(parent, "Power", "--", 0, 0);

    layout.Position(coreTile.GetObject(),    0, 0);
    layout.Position(displayTile.GetObject(), 1, 0);
    layout.Position(touchTile.GetObject(),   2, 0);
    layout.Position(flashTile.GetObject(),   3, 0);

    layout.Position(psramTile.GetObject(),   0, 1);
    layout.Position(heapTile.GetObject(),    1, 1);
    layout.Position(uptimeTile.GetObject(),  2, 1);
    layout.Position(powerTile.GetObject(), 3, 1);
}

void SystemDashboardView::Update()
{
    if (parent == nullptr)
    {
        return;
    }

    if (millis() - lastUpdateMs < 1000)
    {
        return;
    }

    lastUpdateMs = millis();

    char buffer[32];

    coreTile.SetValue("OK");
    displayTile.SetValue("OK");
    touchTile.SetValue("OK");

    snprintf(
        buffer,
        sizeof(buffer),
        "%d MB",
        SystemService::GetFlashSizeMB()
    );
    flashTile.SetValue(buffer);

    psramTile.SetValue(
        SystemService::HasPSRAM() ? "Ready" : "None"
    );

    snprintf(
        buffer,
        sizeof(buffer),
        "%u KB",
        SystemService::GetFreeHeapKB()
    );
    heapTile.SetValue(buffer);

    uptimeTile.SetValue(
        SystemService::GetFormattedUptime().c_str()
    );

    if (PowerService::IsBatteryConnected())
    {
        if (PowerService::IsCharging())
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "CHG %u%%",
                PowerService::GetBatteryPercent()
            );
        }
        else if (PowerService::IsUSBConnected())
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "USB %u%%",
                PowerService::GetBatteryPercent()
            );
        }
        else
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "BAT %u%%",
                PowerService::GetBatteryPercent()
            );
        }
    }
    else if (PowerService::IsUSBConnected())
    {
        snprintf(buffer, sizeof(buffer), "USB");
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "Unknown");
    }

    powerTile.SetValue(buffer);
}