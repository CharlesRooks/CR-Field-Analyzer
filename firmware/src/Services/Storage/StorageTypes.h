#pragma once

#include <stdint.h>

#include "../WiFi/WiFiTypes.h"

struct StoredWiFiMeasurementSession
{
    bool available = false;

    // Monotonically increasing identifier persisted in the
    // session filename. This is used for ordering because the
    // current hardware does not yet expose a wall-clock time.
    uint32_t sessionId = 0;

    // Uptime timestamp captured when the measurement completed.
    // It is useful within the original boot, but is not a date/time.
    uint32_t completedAtMs = 0;

    WiFiMeasurementSummary summary{};
};
