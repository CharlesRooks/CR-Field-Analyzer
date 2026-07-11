#pragma once

#include "../../Core/ScreenID.h"

#include <lvgl.h>
#include <stdint.h>

class HeaderBar
{
public:
    void Show(ScreenID current);
    void SetCurrent(ScreenID current);
    void Update();

private:
    lv_obj_t *brandLabel = nullptr;
    lv_obj_t *pageLabel = nullptr;
    lv_obj_t *powerLabel = nullptr;
};