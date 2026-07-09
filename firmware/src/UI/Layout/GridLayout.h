#pragma once

#include <lvgl.h>

class GridLayout
{
public:
    GridLayout(lv_obj_t *parent, int columns, int tileWidth, int tileHeight, int gap);

    void Position(lv_obj_t *object, int column, int row);

private:
    lv_obj_t *parent = nullptr;
    int columns = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int gap = 0;
};