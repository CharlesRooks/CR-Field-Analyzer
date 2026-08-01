#pragma once

#include <stdint.h>
#include "../ScreenID.h"

// Provides the InputEvent enum
#include "../../Managers/InputManager.h"

enum class MessageType : uint8_t
{
    None = 0,

    // System
    UserActivity,

    // Input
    InputEvent,
    NavigationChanged,

    // Power
    ChargingStarted,
    ChargingStopped,
    BatteryLevelChanged,
    BatteryLow,

    // Display
    DisplayDimmed,
    DisplayTurnedOn,
    DisplayTurnedOff,

    // Sleep
    SleepEntering,
    SleepExited,

    // Applications
    ApplicationChanged,

    // WiFi
    WiFiScanStarted,
    WiFiScanCompleted,

    // Notifications
    NotificationRaised
};

struct Message
{
    MessageType type = MessageType::None;

    uint32_t timestampMs = 0;

    union
    {
        InputEvent inputEvent;
        ScreenID screenId;

        int32_t value1;
        uint32_t value2;

        bool flag;
    };
};