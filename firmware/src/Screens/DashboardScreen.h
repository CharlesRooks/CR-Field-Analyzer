#pragma once

#include "../Core/Page.h"
#include "../UI/Views/SystemDashboardView.h"

class DashboardScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;

private:
    SystemDashboardView systemDashboard;
};