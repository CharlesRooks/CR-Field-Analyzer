#include "DashboardScreen.h"
#include "../UI/Theme.h"
#include <lvgl.h>

namespace DashboardScreen
{
    void Show()
    {
        Theme::PrepareScreen();

        lv_obj_t *header = lv_label_create(lv_scr_act());
        lv_label_set_text(header, "SentinelOS");
        lv_obj_set_style_text_color(header, Theme::Accent(), 0);
        lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
        lv_obj_align(header, LV_ALIGN_TOP_LEFT, 10, 10);

        lv_obj_t *status = lv_label_create(lv_scr_act());
        lv_label_set_text(status,
            "Core      : OK\n"
            "Display   : OK\n"
            "Touch     : OK\n"
            "Flash     : 16 MB\n"
            "PSRAM     : Ready\n"
            "WiFi      : Ready");

        lv_obj_set_style_text_color(status, Theme::Text(), 0);
        lv_obj_align(status, LV_ALIGN_TOP_LEFT, 20, 60);

        lv_obj_t *footer = lv_label_create(lv_scr_act());
        lv_label_set_text(footer, "Dashboard   Scan   Tools   Settings");
        lv_obj_set_style_text_color(footer, Theme::Muted(), 0);
        lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);

        lv_timer_handler();
    }
}