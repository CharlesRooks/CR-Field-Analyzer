#include "HeaderBar.h"
#include "../Theme.h"

#include <lvgl.h>

static const char *GetPageTitle(ScreenID screen)
{
    switch (screen)
    {
        case ScreenID::Dashboard: return "Dashboard";
        case ScreenID::Scan:      return "Wi-Fi Scan";
        case ScreenID::Tools:     return "Tools";
        case ScreenID::Settings:  return "Settings";
        default:                  return "Dashboard";
    }
}

void HeaderBar::Show(ScreenID current)
{
    
    if (brandLabel != nullptr)
    {
        lv_obj_del(brandLabel);
        brandLabel = nullptr;
    }

    if (pageLabel != nullptr)
    {
        lv_obj_del(pageLabel);
        pageLabel = nullptr;
    }
    
    brandLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(brandLabel, "SentinelOS");
    lv_obj_set_style_text_color(brandLabel, Theme::Accent(), 0);
    lv_obj_set_style_text_font(brandLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(brandLabel, LV_ALIGN_TOP_LEFT, 10, 8);

    pageLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(pageLabel, Theme::Text(), 0);
    lv_obj_align(pageLabel, LV_ALIGN_TOP_LEFT, 10, 36);

    SetCurrent(current);
}

void HeaderBar::SetCurrent(ScreenID current)
{
    if (pageLabel != nullptr)
    {
        lv_label_set_text(pageLabel, GetPageTitle(current));
    }
}