#include "ScanScreen.h"
#include "../UI/Theme.h"
#include <lvgl.h>

void ScanScreen::CreateContent()
{
    lv_obj_t *parent = GetContentArea();

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Wi-Fi Scan");
    lv_obj_set_style_text_color(title, Theme::Accent(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *message = lv_label_create(parent);
    lv_label_set_text(message, "Scanner module coming soon");
    lv_obj_set_style_text_color(message, Theme::Text(), 0);
    lv_obj_align(message, LV_ALIGN_CENTER, 0, 0);
}

void ScanScreen::Update()
{
}