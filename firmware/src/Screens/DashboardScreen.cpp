#include "DashboardScreen.h"
#include "../Services/System/SystemService.h"
#include <Arduino.h>
#include <lvgl.h>

void DashboardScreen::CreateContent()
{
    statusLabel = lv_label_create(GetContentArea());
    lv_obj_set_style_text_color(statusLabel, Theme::Text(), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 10, 10);

    Update();
}

void DashboardScreen::Update()
{
    if (millis() - lastUpdateMs < 1000)
    {
        return;
    }

    lastUpdateMs = millis();
    updateCounter++;

    char buffer[256];

    snprintf(buffer, sizeof(buffer),
             "Core      : OK\n"
             "Display   : OK\n"
             "Touch     : OK\n"
             "Flash     : %d MB\n"
             "PSRAM     : %s\n"
             "Heap      : %u KB\n"
             "Uptime    : %s\n"
             "Updates   : %lu",
             SystemService::GetFlashSizeMB(),
             SystemService::HasPSRAM() ? "Ready" : "Not Found",
             SystemService::GetFreeHeapKB(),
             SystemService::GetFormattedUptime().c_str(),
             updateCounter);

    lv_label_set_text(statusLabel, buffer);
}