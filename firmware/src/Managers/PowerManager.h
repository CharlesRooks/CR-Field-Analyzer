#pragma once

#include "PowerPolicy.h"
#include <stdint.h>
#include "../Core/Messaging/MessageTypes.h"

enum class DisplayPowerState : uint8_t
{
    Active,
    Dimmed,
    Off
};
class PowerManager
{
public:
    static void Begin();
    static void Update();
    static const PowerPolicy &GetActivePolicy();
    static uint32_t GetIdleTimeMs();

private:
    static PowerPolicy usbPolicy;
    static PowerPolicy batteryPolicy;
    static const PowerPolicy *activePolicy;

    static uint32_t lastActivityMs;
    static DisplayPowerState displayState;

    static void NotifyActivity();
    static void SelectActivePolicy();
    static void ApplyDisplayPolicy();
    static void HandleMessage(const Message &message);
};