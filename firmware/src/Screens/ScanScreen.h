#pragma once

#include "../Core/Page.h"

class ScanScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;
};