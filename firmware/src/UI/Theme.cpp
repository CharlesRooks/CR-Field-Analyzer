#include "Theme.h"

namespace Theme
{
    lv_color_t Background()
    {
        return lv_color_black();
    }

    lv_color_t Text()
    {
        return lv_color_white();
    }

    lv_color_t Accent()
    {
        return lv_palette_main(LV_PALETTE_RED);
    }

    lv_color_t Muted()
    {
        return lv_palette_main(LV_PALETTE_GREY);
    }

    void PrepareScreen()
    {
        lv_obj_clean(lv_scr_act());

        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_style_bg_color(lv_scr_act(), Background(), 0);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    }
}