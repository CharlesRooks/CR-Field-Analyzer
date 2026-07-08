#include "HeaderBar.h"
#include "../Theme.h"

#include <lvgl.h>

static const char *GetPageTitle(ScreenID screen)
{
    switch (screen)
    {
        case ScreenID::Dashboard:
            return "Dashboard";

        case ScreenID::Scan:
            return "Wi-Fi Scan";

        case ScreenID::Tools:
            return "Tools";

        case ScreenID::Settings:
            return "Settings";

        default:
            return "Dashboard";
    }
}

void HeaderBar::Draw(ScreenID current)
{
    lv_obj_t *brand = lv_label_create(lv_scr_act());
    lv_label_set_text(brand, "SentinelOS");
    lv_obj_set_style_text_color(brand, Theme::Accent(), 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_20, 0);
    lv_obj_align(brand, LV_ALIGN_TOP_LEFT, 10, 8);

    lv_obj_t *page = lv_label_create(lv_scr_act());
    lv_label_set_text(page, GetPageTitle(current));
    lv_obj_set_style_text_color(page, Theme::Text(), 0);
    lv_obj_align(page, LV_ALIGN_TOP_LEFT, 10, 36);
}