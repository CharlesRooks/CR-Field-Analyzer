#pragma once

#include <lvgl.h>

namespace Theme
{
    void PrepareScreen();

    lv_color_t Background();
    lv_color_t Text();
    lv_color_t Accent();
    lv_color_t Muted();

    // Repeated interaction controls use a quieter palette than the
    // SentinelOS brand accent. Red remains available through Accent()
    // for branding, warnings, and critical states.
    lv_color_t Control();
    lv_color_t ControlPressed();
    lv_color_t ControlSelected();
    lv_color_t ControlDisabled();
    lv_color_t ControlText();
}