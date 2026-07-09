#include "GridLayout.h"

GridLayout::GridLayout(lv_obj_t *parent, int columns, int tileWidth, int tileHeight, int gap)
{
    this->parent = parent;
    this->columns = columns;
    this->tileWidth = tileWidth;
    this->tileHeight = tileHeight;
    this->gap = gap;
}

void GridLayout::Position(lv_obj_t *object, int column, int row)
{
    if (parent == nullptr || object == nullptr)
    {
        return;
    }

    int x = column * (tileWidth + gap);
    int y = row * (tileHeight + gap);

    lv_obj_align(object, LV_ALIGN_TOP_LEFT, x, y);
}