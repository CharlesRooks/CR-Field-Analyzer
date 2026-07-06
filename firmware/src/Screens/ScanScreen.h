#pragma once

#include "../Core/Screen.h"

class ScanScreen : public Screen
{
public:
    void Show() override;
    void Update() override;
    void Hide() override;
};