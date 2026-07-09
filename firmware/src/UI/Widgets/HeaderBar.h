#pragma once

#include "../../Core/ScreenID.h"
#include <lvgl.h>

class HeaderBar
{
public:
    void Show(ScreenID current);
    void SetCurrent(ScreenID current);

private:
    lv_obj_t *brandLabel = nullptr;
    lv_obj_t *pageLabel = nullptr;
};