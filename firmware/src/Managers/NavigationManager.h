#pragma once

#include "../Core/ScreenID.h"
#include "../Core/Page.h"

#include "../Screens/DashboardScreen.h"
#include "../Screens/ScanScreen.h"
#include "../Screens/ToolsScreen.h"
#include "../Screens/SettingsScreen.h"

#include <lvgl.h>

class NavigationManager
{
public:
    void Begin(lv_obj_t *contentArea);

    void Show(ScreenID screen);

    void Next();
    void Previous();

    void Update();

    ScreenID Current() const
    {
        return current;
    }

private:
    lv_obj_t *contentArea = nullptr;

    DashboardScreen dashboard;
    ScanScreen scan;
    ToolsScreen tools;
    SettingsScreen settings;

    Page *currentScreen = nullptr;
    ScreenID current = ScreenID::Dashboard;
};