#include "ApplicationFrame.h"
#include "Theme.h"

void ApplicationFrame::Show(ScreenID currentScreen)
{
    Theme::PrepareScreen();

    headerBar.Show(currentScreen);

    CreateSeparators();

    contentArea = lv_obj_create(lv_scr_act());
    lv_obj_set_size(contentArea, 220, 390);
    lv_obj_align(contentArea, LV_ALIGN_TOP_LEFT, 10, 70);

    lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(contentArea, 0, 0);
    lv_obj_set_style_pad_all(contentArea, 0, 0);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    navigationBar.Show();
    navigationBar.SetCurrent(currentScreen);
}

void ApplicationFrame::SetCurrent(ScreenID currentScreen)
{
    headerBar.SetCurrent(currentScreen);
    navigationBar.SetCurrent(currentScreen);
}

lv_obj_t *ApplicationFrame::GetContentArea()
{
    return contentArea;
}

void ApplicationFrame::CreateSeparators()
{
    lv_obj_t *topLine = lv_obj_create(lv_scr_act());
    lv_obj_set_size(topLine, 220, 1);
    lv_obj_align(topLine, LV_ALIGN_TOP_LEFT, 10, 63);
    lv_obj_set_style_bg_color(topLine, Theme::Muted(), 0);
    lv_obj_set_style_bg_opa(topLine, LV_OPA_50, 0);
    lv_obj_set_style_border_width(topLine, 0, 0);
    lv_obj_clear_flag(topLine, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bottomLine = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottomLine, 220, 1);
    lv_obj_align(bottomLine, LV_ALIGN_BOTTOM_LEFT, 10, -31);
    lv_obj_set_style_bg_color(bottomLine, Theme::Muted(), 0);
    lv_obj_set_style_bg_opa(bottomLine, LV_OPA_50, 0);
    lv_obj_set_style_border_width(bottomLine, 0, 0);
    lv_obj_clear_flag(bottomLine, LV_OBJ_FLAG_SCROLLABLE);
}