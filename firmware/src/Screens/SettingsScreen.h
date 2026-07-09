#pragma once

#include "../Core/Page.h"

class SettingsScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;
};