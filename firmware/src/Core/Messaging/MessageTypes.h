#pragma once

#include <stdint.h>

#include "../ScreenID.h"
#include "../InputEvent.h"


enum class MessageType : uint8_t
{
    None = 0,

    // Active system messages
    UserActivity,

    // Active input and navigation messages
    InputEvent,
    NavigationChanged,

    // Reserved: power events
    ChargingStarted,
    ChargingStopped,
    BatteryLevelChanged,
    BatteryLow,

    // Reserved: display events
    DisplayDimmed,
    DisplayTurnedOn,
    DisplayTurnedOff,

    // Reserved: sleep events
    SleepEntering,
    SleepExited,

    // Reserved: application events
    ApplicationChanged,

    // Active Wi-Fi messages
    WiFiScanStarted,
    WiFiScanCompleted,
    WiFiMeasurementSessionCompleted,

    // Reserved: notification events
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

        uint8_t wifiNetworkCount;

        int32_t value1;
        uint32_t value2;

        bool flag;
    };
};