#include "HeaderBar.h"
#include "../Theme.h"
#include "../../Services/Power/PowerService.h"

#include <Arduino.h>
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

void HeaderBar::Show(ScreenID current)
{
    brandLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(brandLabel, "SentinelOS");
    lv_obj_set_style_text_color(brandLabel, Theme::Accent(), 0);
    lv_obj_set_style_text_font(brandLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(brandLabel, LV_ALIGN_TOP_LEFT, 10, 8);

    powerLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(powerLabel, Theme::Text(), 0);
    lv_obj_align(powerLabel, LV_ALIGN_TOP_RIGHT, -10, 10);

    pageLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(pageLabel, Theme::Text(), 0);
    lv_obj_align(pageLabel, LV_ALIGN_TOP_LEFT, 10, 36);

    SetCurrent(current);

    Update();
}

void HeaderBar::SetCurrent(ScreenID current)
{
    if (pageLabel != nullptr)
    {
        lv_label_set_text(pageLabel, GetPageTitle(current));
    }
}

void HeaderBar::Update()
{
    if (powerLabel == nullptr)
    {
        return;
    }

    char buffer[24];

    PowerService::FormatStatus(
        buffer,
        sizeof(buffer)
    );

    lv_label_set_text(powerLabel, buffer);
}