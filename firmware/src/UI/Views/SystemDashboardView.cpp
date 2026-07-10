#include "SystemDashboardView.h"

#include "../../Services/System/SystemService.h"

#include <Arduino.h>

void SystemDashboardView::Create(lv_obj_t *parentObject)
{
    parent = parentObject;

    GridLayout layout(parent, 4, 100, 48, 10);

    coreTile.Create(
        parent,
        "Core",
        "OK",
        0,
        0
    );

    layout.Position(
        coreTile.GetObject(),
        0,
        0
    );
}

void SystemDashboardView::Update()
{
    if (parent == nullptr)
    {
        return;
    }

    if (millis() - lastUpdateMs < 1000)
    {
        return;
    }

    lastUpdateMs = millis();

    coreTile.SetValue("OK");
}