#include "NavigationManager.h"

void NavigationManager::Show(ScreenID screen)
{
    if (currentScreen != nullptr)
    {
        currentScreen->Hide();
    }

    current = screen;

    switch (screen)
    {
        case ScreenID::Dashboard:
            currentScreen = &dashboard;
            break;

        case ScreenID::Scan:
            currentScreen = &scan;
            break;

        case ScreenID::Tools:
            currentScreen = &tools;
            break;

        case ScreenID::Settings:
            currentScreen = &settings;
            break;

        default:
            currentScreen = &dashboard;
            current = ScreenID::Dashboard;
            break;
    }

    currentScreen->Show();

    navigationBar.Show();
    navigationBar.SetCurrent(current);
}

void NavigationManager::Next()
{
    int next = static_cast<int>(current) + 1;

    if (next >= static_cast<int>(ScreenID::Count))
    {
        next = 0;
    }

    Show(static_cast<ScreenID>(next));
}

void NavigationManager::Previous()
{
    int previous = static_cast<int>(current) - 1;

    if (previous < 0)
    {
        previous = static_cast<int>(ScreenID::Count) - 1;
    }

    Show(static_cast<ScreenID>(previous));
}

void NavigationManager::Update()
{
    if (currentScreen != nullptr)
    {
        currentScreen->Update();
    }
}