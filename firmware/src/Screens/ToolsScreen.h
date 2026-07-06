#pragma once

#include "../Core/Screen.h"

class ToolsScreen : public Screen
{
public:
    void Show() override;
    void Update() override;
    void Hide() override;
};