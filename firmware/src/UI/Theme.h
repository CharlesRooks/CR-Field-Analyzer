#pragma once

#include <lvgl.h>

namespace Theme
{
    void PrepareScreen();

    lv_color_t Background();
    lv_color_t Text();
    lv_color_t Accent();
    lv_color_t Muted();
}