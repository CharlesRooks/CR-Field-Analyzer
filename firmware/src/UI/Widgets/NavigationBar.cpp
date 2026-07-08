#include "NavigationBar.h"
#include "../Theme.h"

static int ScreenToIndex(ScreenID screen)
{
    return static_cast<int>(screen);
}

void NavigationBar::Show()
{
    bar = lv_obj_create(lv_scr_act());

    lv_obj_set_size(bar, 240, 28);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_set_style_bg_color(bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    const int startX = 78;
    const int spacing = 28;

    for (int i = 0; i < 4; i++)
    {
        dots[i] = lv_obj_create(bar);

        lv_obj_set_size(dots[i], 10, 10);
        lv_obj_set_style_radius(dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(dots[i], 0, 0);

        lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_align(dots[i], LV_ALIGN_LEFT_MID, startX + (i * spacing), 0);
    }

    SetCurrent(ScreenID::Dashboard);
}

void NavigationBar::SetCurrent(ScreenID current)
{
    int activeIndex = ScreenToIndex(current);

    for (int i = 0; i < 4; i++)
    {
        SetDotState(dots[i], i == activeIndex);
    }
}

void NavigationBar::SetDotState(lv_obj_t *dot, bool active)
{
    if (dot == nullptr)
    {
        return;
    }

    if (active)
    {
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_bg_color(dot, Theme::Accent(), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
    }
    else
    {
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(dot, 1, 0);
        lv_obj_set_style_border_color(dot, Theme::Muted(), 0);
    }
}