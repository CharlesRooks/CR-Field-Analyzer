#pragma once

#include "../Screens/DashboardScreen.h"

class SentinelOS
{
public:
    void Begin();
    void Update();

private:
    DashboardScreen dashboard;
};