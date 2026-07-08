#pragma once

#include "Widgets/HeaderBar.h"
#include "Widgets/NavigationBar.h"
#include "../Core/ScreenID.h"
#include <lvgl.h>

class ApplicationFrame
{
public:
    void Show(ScreenID currentScreen);
    void SetCurrent(ScreenID currentScreen);

    lv_obj_t *GetContentArea();

private:
    lv_obj_t *contentArea = nullptr;

    HeaderBar headerBar;
    NavigationBar navigationBar;

    void CreateSeparators();
};