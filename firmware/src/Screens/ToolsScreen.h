#pragma once

#include "../Core/Page.h"

class ToolsScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;
};