#pragma once

#include <stdint.h>

#include "../WiFi/WiFiTypes.h"

struct StoredWiFiMeasurementSession
{
    bool available = false;

    // Version of the persisted text record. Version 1 is the
    // original 10.15B format; version 2 adds CRC32 integrity.
    uint8_t formatVersion = 0;

    // True only when the stored record carried a checksum and
    // the checksum was verified successfully while loading.
    bool integrityVerified = false;

    // Monotonically increasing identifier persisted in the
    // session filename. This is used for ordering because the
    // current hardware does not yet expose a wall-clock time.
    uint32_t sessionId = 0;

    // Uptime timestamp captured when the measurement completed.
    // It is useful within the original boot, but is not a date/time.
    uint32_t completedAtMs = 0;

    WiFiMeasurementSummary summary{};
};
