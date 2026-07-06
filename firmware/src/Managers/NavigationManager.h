#pragma once

#include "../Screens/SplashScreen.h"
#include "../Screens/DashboardScreen.h"
#include "../Screens/ScanScreen.h"
#include "../Screens/ToolsScreen.h"
#include "../Screens/SettingsScreen.h"

enum class ScreenID
{
    Splash,
    Dashboard,
    Scan,
    Tools,
    Settings
};

class NavigationManager
{
public:
    void Show(ScreenID screen);
    void Update();

private:
    DashboardScreen dashboard;
    ScanScreen scan;
    ToolsScreen tools;
    SettingsScreen settings;

    Screen* currentScreen = nullptr;
};