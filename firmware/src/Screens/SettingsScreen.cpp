#include "SettingsScreen.h"
#include "../UI/Theme.h"
#include <lvgl.h>

void SettingsScreen::Show()
{
    Theme::PrepareScreen();

    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, Theme::Accent(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *message = lv_label_create(lv_scr_act());
    lv_label_set_text(message, "Settings module coming soon");
    lv_obj_set_style_text_color(message, Theme::Text(), 0);
    lv_obj_align(message, LV_ALIGN_CENTER, 0, 0);
}

void SettingsScreen::Update()
{
}

void SettingsScreen::Hide()
{
    lv_obj_clean(lv_scr_act());
}