#include "NavigationManager.h"

void NavigationManager::Show(ScreenID screen)
{
    if (currentScreen != nullptr)
    {
        currentScreen->Hide();
    }

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
            return;
    }

    currentScreen->Show();
}

void NavigationManager::Update()
{
    if (currentScreen != nullptr)
    {
        currentScreen->Update();
    }
}