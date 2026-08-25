#pragma once

#include "../Core/ScreenID.h"
#include "../Core/Page.h"

#include "../Screens/DashboardScreen.h"
#include "../Screens/ScanScreen.h"
#include "../Screens/ToolsScreen.h"
#include "../Screens/SettingsScreen.h"
#include "../Core/Messaging/MessageTypes.h"

#include <lvgl.h>

class NavigationManager
{
public:
    void Begin(lv_obj_t *contentArea);

    void Show(ScreenID screen);

    void Next();
    void Previous();

    void Update();

    // Modal UI workflows can temporarily suppress raw horizontal swipe
    // navigation. This is required for interactive Floor Plan panning,
    // where the same physical gesture must move the map rather than the
    // application page underneath it.
    static void SetGestureNavigationEnabled(bool enabled);
    static bool IsGestureNavigationEnabled();

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
    static void HandleMessage(const Message &message);
    static NavigationManager *instance;
    static bool gestureNavigationEnabled;
};