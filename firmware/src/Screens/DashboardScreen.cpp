#include "DashboardScreen.h"
#include "../UI/Theme.h"
#include <Arduino.h>
#include <lvgl.h>

void DashboardScreen::Show()
{
    Theme::PrepareScreen();

    lv_obj_t *header = lv_label_create(lv_scr_act());
    lv_label_set_text(header, "SentinelOS");
    lv_obj_set_style_text_color(header, Theme::Accent(), 0);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 10, 10);

    statusLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(statusLabel, Theme::Text(), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 20, 60);

    lv_obj_t *footer = lv_label_create(lv_scr_act());
    lv_label_set_text(footer, "Dashboard   Scan   Tools   Settings");
    lv_obj_set_style_text_color(footer, Theme::Muted(), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);

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

    uint32_t totalSeconds = millis() / 1000;
    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;

    char buffer[256];

    snprintf(buffer, sizeof(buffer),
             "Core      : OK\n"
             "Display   : OK\n"
             "Touch     : OK\n"
             "Flash     : %d MB\n"
             "PSRAM     : %s\n"
             "Heap      : %u KB\n"
             "Uptime    : %02lu:%02lu:%02lu\n"
             "Updates   : %lu",
             ESP.getFlashChipSize() / (1024 * 1024),
             psramFound() ? "Ready" : "Not Found",
             ESP.getFreeHeap() / 1024,
             hours,
             minutes,
             seconds,
             updateCounter);

    lv_label_set_text(statusLabel, buffer);
}