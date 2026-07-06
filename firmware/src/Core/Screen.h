#pragma once

class Screen
{
public:
    virtual ~Screen() = default;

    virtual void Show() = 0;

    virtual void Update() = 0;

    virtual void Hide() {}
};