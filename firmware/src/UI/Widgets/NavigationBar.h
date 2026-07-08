#pragma once

#include "../../Core/ScreenID.h"
#include <lvgl.h>

class NavigationBar
{
public:
    void Show();
    void SetCurrent(ScreenID current);

private:
    lv_obj_t *bar = nullptr;
    lv_obj_t *dots[4] = {nullptr, nullptr, nullptr, nullptr};

    void SetDotState(lv_obj_t *dot, bool active);
};