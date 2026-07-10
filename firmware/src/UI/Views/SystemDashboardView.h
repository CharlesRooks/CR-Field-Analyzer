#pragma once

#include "../Widgets/StatusTile.h"
#include "../Layout/GridLayout.h"

#include <lvgl.h>
#include <stdint.h>

class SystemDashboardView
{
public:
    void Create(lv_obj_t *parent);
    void Update();

private:
    lv_obj_t *parent = nullptr;

    StatusTile coreTile;

    uint32_t lastUpdateMs = 0;
};