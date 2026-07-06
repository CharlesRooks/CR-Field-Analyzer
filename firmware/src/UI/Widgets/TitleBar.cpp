#include "TitleBar.h"
#include "../Theme.h"
#include <lvgl.h>
#include <Arduino.h>

void TitleBar::Draw()
{
    Serial.println("Drawing TitleBar");

    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "SentinelOS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, Theme::Accent(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);
}