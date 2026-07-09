#include "StatusTile.h"
#include "../Theme.h"

void StatusTile::Create(lv_obj_t *parent, const char *label, const char *value, int x, int y)
{
    container = lv_obj_create(parent);
    lv_obj_set_size(container, 100, 48);
    // Position will be handled by GridLayout

    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_border_color(container, Theme::Muted(), 0);
    lv_obj_set_style_radius(container, 6, 0);
    lv_obj_set_style_pad_all(container, 6, 0);

    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    labelText = lv_label_create(container);
    lv_label_set_text(labelText, label);
    lv_obj_set_style_text_color(labelText, Theme::Muted(), 0);
    lv_obj_align(labelText, LV_ALIGN_TOP_LEFT, 0, 0);

    valueText = lv_label_create(container);
    lv_label_set_text(valueText, value);
    lv_obj_set_style_text_color(valueText, Theme::Text(), 0);
    lv_obj_align(valueText, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

void StatusTile::SetValue(const char *value)
{
    if (valueText != nullptr)
    {
        lv_label_set_text(valueText, value);
    }
}