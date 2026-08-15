#pragma once

#include <stdint.h>

#include "../WiFi/WiFiTypes.h"

struct StoredWiFiMeasurementSession
{
    bool available = false;

    // Version of the persisted text record. Version 1 is the
    // original 10.15B format; version 2 adds CRC32 integrity;
    // version 3 adds a wall-clock capture timestamp; version 4
    // adds the per-BSSID network inventory; version 5 adds the
    // optional site-survey point label.
    uint8_t formatVersion = 0;

    // True only when the stored record carried a checksum and
    // the checksum was verified successfully while loading.
    bool integrityVerified = false;

    // Monotonically increasing identifier persisted in the
    // session filename. This remains the stable storage ordering
    // key even when a wall-clock capture timestamp is available.
    uint32_t sessionId = 0;

    // True when the session includes a valid wall-clock capture
    // timestamp. Legacy version 1 and version 2 records leave this
    // false because they only stored uptime.
    bool capturedTimeValid = false;

    // Unix epoch seconds captured when the measurement session
    // completed. SentinelOS currently operates in local UTC-04:00,
    // while epoch time itself remains timezone independent.
    uint32_t capturedEpoch = 0;

    static constexpr uint8_t CapturedLocalCapacity = 32;

    // Human-readable ISO local timestamp persisted alongside epoch
    // for convenient inspection of the text record on a computer.
    char capturedLocal[CapturedLocalCapacity] = {};

    // Uptime timestamp captured when the measurement completed.
    // Retained as diagnostic metadata even after wall-clock support.
    uint32_t completedAtMs = 0;

    WiFiMeasurementSummary summary{};
};

struct StoredWiFiMeasurementSessionIndex
{
    bool available = false;

    // The SD-card filename remains the authoritative ordering key.
    uint32_t sessionId = 0;

    // Metadata is populated lazily the first time the full session
    // is opened. Startup indexing therefore avoids loading every
    // detailed measurement record into RAM.
    bool metadataLoaded = false;
    bool capturedTimeValid = false;
    uint32_t capturedEpoch = 0;
    uint8_t networkCount = 0;
    uint8_t observedNetworkCount = 0;
    uint8_t bestChannel = 0;
    WiFiRecommendationConfidence confidence =
        WiFiRecommendationConfidence::Unknown;
};

