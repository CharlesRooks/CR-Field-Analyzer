#pragma once

#include "../Core/Page.h"
#include <lvgl.h>
#include <stdint.h>

class DashboardScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;

private:
    lv_obj_t *statusLabel = nullptr;
    uint32_t lastUpdateMs = 0;
    uint32_t updateCounter = 0;
};