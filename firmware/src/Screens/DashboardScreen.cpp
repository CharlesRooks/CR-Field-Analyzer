#include "DashboardScreen.h"

void DashboardScreen::CreateContent()
{
    systemDashboard.Create(GetContentArea());
}

void DashboardScreen::Update()
{
    systemDashboard.Update();
}