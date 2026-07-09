#pragma once

#include "Screen.h"
#include <lvgl.h>

class Page : public Screen
{
public:
    void Show() override
    {
        CreateContent();
    }

    void Show(lv_obj_t *parent)
    {
        contentArea = parent;
        CreateContent();
    }

    void Hide() override
    {
        if (contentArea != nullptr)
        {
            lv_obj_clean(contentArea);
        }
    }

protected:
    lv_obj_t *GetContentArea()
    {
        return contentArea;
    }

    virtual void CreateContent() = 0;

private:
    lv_obj_t *contentArea = nullptr;
};