#pragma once

#include <lvgl.h>

class StatusTile
{
public:
    void Create(lv_obj_t *parent,
                const char *label,
                const char *value,
                int x,
                int y);

    void SetValue(const char *value);

    lv_obj_t *GetObject()
    {
        return container;
    }

private:
    lv_obj_t *container = nullptr;
    lv_obj_t *labelText = nullptr;
    lv_obj_t *valueText = nullptr;
};