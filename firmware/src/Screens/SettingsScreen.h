#pragma once

#include "../Core/Screen.h"

class SettingsScreen : public Screen
{
public:
    void Show() override;
    void Update() override;
    void Hide() override;
};