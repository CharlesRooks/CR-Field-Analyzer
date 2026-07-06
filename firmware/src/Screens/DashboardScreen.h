#pragma once

#include <lvgl.h>
#include <stdint.h>

class DashboardScreen
{
public:
    void Show();
    void Update();

private:
    lv_obj_t *statusLabel = nullptr;
    uint32_t lastUpdateMs = 0;
    uint32_t updateCounter = 0;
};