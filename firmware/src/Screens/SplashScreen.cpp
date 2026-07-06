#include "SplashScreen.h"
#include "../UI/Theme.h"
#include <lvgl.h>

namespace SplashScreen
{
    void Show()
    {
        Theme::PrepareScreen();

        lv_obj_t *title = lv_label_create(lv_scr_act());
        lv_label_set_text(title, "SentinelOS");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(title, Theme::Accent(), 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);

        lv_obj_t *subtitle = lv_label_create(lv_scr_act());
        lv_label_set_text(subtitle, "CR Field Analyzer");
        lv_obj_set_style_text_color(subtitle, Theme::Text(), 0);
        lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t *version = lv_label_create(lv_scr_act());
        lv_label_set_text(version, "Version 0.2 Alpha");
        lv_obj_set_style_text_color(version, Theme::Muted(), 0);
        lv_obj_align(version, LV_ALIGN_CENTER, 0, 20);

        lv_timer_handler();
    }
}