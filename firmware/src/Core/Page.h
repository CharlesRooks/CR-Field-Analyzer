#pragma once

#include "Screen.h"
#include "../UI/Theme.h"
#include "../UI/Widgets/TitleBar.h"
#include <lvgl.h>

class Page : public Screen
{
public:
    void Show() override
    {
        Theme::PrepareScreen();

        contentArea = lv_obj_create(lv_scr_act());
        lv_obj_set_size(contentArea, 220, 430);
        lv_obj_align(contentArea, LV_ALIGN_TOP_LEFT, 10, 45);

        lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_bg_color(contentArea, Theme::Background(), 0);
        lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(contentArea, 0, 0);
        lv_obj_set_style_pad_all(contentArea, 0, 0);

        CreateContent();

        TitleBar::Draw();
    }

    void Hide() override
    {
        lv_obj_clean(lv_scr_act());
        contentArea = nullptr;
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