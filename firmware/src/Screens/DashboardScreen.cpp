#include "DashboardScreen.h"
#include "../UI/Theme.h"
#include "../UI/Layout/GridLayout.h"
#include "../Services/System/SystemService.h"
#include <Arduino.h>
#include <lvgl.h>


void DashboardScreen::CreateContent()
{
    // Existing dashboard text
    statusLabel = lv_label_create(GetContentArea());
    lv_obj_set_style_text_color(statusLabel, Theme::Text(), 0);

    // Move the old text to the right so it doesn't overlap the tile
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 125, 0);

    // Create the grid
    GridLayout layout(GetContentArea(), 4, 100, 48, 10);

    // Create the first tile
    coreTile.Create(GetContentArea(),
                    "Core",
                    "OK",
                    0,
                    0);

    // Position the tile
    layout.Position(coreTile.GetObject(), 0, 0);

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
             "Uptime    : %s\n",
             SystemService::GetFlashSizeMB(),
             SystemService::HasPSRAM() ? "Ready" : "Not Found",
             SystemService::GetFreeHeapKB(),
             SystemService::GetFormattedUptime().c_str()
             );

    lv_label_set_text(statusLabel, buffer);
}