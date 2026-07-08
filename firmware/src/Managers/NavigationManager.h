#pragma once

#include "../Core/ScreenID.h"

#include "../Screens/DashboardScreen.h"
#include "../Screens/ScanScreen.h"
#include "../Screens/ToolsScreen.h"
#include "../Screens/SettingsScreen.h"

#include "../UI/Widgets/NavigationBar.h"

class NavigationManager
{
public:
    void Show(ScreenID screen);

    void Next();
    void Previous();

    void Update();

    ScreenID Current() const
    {
        return current;
    }

private:
    DashboardScreen dashboard;
    ScanScreen scan;
    ToolsScreen tools;
    SettingsScreen settings;

    NavigationBar navigationBar;

    Screen *currentScreen = nullptr;
    ScreenID current = ScreenID::Dashboard;
};