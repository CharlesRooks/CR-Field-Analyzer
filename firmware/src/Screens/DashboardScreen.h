#pragma once

#include "../Core/Screen.h"
#include <lvgl.h>
#include <stdint.h>

class DashboardScreen : public Screen
{
public:
    void Show() override;
    void Update() override;
    void Hide() override;

private:
    lv_obj_t *statusLabel = nullptr;
    uint32_t lastUpdateMs = 0;
    uint32_t updateCounter = 0;
};