#pragma once

#include <stdint.h>

struct SiteSurveyInfo
{
    static constexpr uint8_t NameCapacity = 48;

    bool active = false;

    // Stable identifier for the survey. Storage integration will
    // assign and persist this in a later 10.22 step.
    uint32_t surveyId = 0;

    // Human-readable survey name, for example:
    // "Hyatt Regency - Level 2".
    char name[NameCapacity] = {};

    // Wall-clock creation time. Zero means unavailable.
    uint32_t createdEpoch = 0;
};