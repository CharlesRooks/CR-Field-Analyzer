#pragma once

#include <stdint.h>

enum class WiFiScanState : uint8_t
{
    Idle = 0,
    Scanning,
    Complete,
    Failed
};

enum class WiFiMeasurementSessionState : uint8_t
{
    Idle = 0,
    Running,
    Cancelling,
    Complete,
    Cancelled,
    Failed
};

enum class WiFiSignalQuality : uint8_t
{
    Unknown = 0,
    Poor,
    Fair,
    Good,
    Excellent
};

enum class WiFiCongestionLevel : uint8_t
{
    Unknown = 0,
    Poor,
    Fair,
    Good,
    Excellent
};

enum class WiFiRecommendationConfidence : uint8_t
{
    Unknown = 0,
    Low,
    Medium,
    High
};

enum class WiFiSecurity : uint8_t
{
    Unknown = 0,
    Open,
    WEP,
    WPA_PSK,
    WPA2_PSK,
    WPA_WPA2_PSK,
    WPA2_Enterprise,
    WPA3_PSK,
    WPA2_WPA3_PSK,
    WAPI_PSK
};

struct WiFiNetworkInfo
{
    static constexpr uint8_t SsidCapacity = 33;
    static constexpr uint8_t BssidLength = 6;

    char ssid[SsidCapacity] = {};
    uint8_t bssid[BssidLength] = {};

    int32_t rssi = -127;
    uint8_t channel = 0;

    WiFiSignalQuality signalQuality =
        WiFiSignalQuality::Unknown;

    WiFiSecurity security =
        WiFiSecurity::Unknown;

    bool hidden = false;
};

struct WiFiMeasuredNetwork
{
    static constexpr uint8_t SsidCapacity =
        WiFiNetworkInfo::SsidCapacity;

    static constexpr uint8_t BssidLength =
        WiFiNetworkInfo::BssidLength;

    char ssid[SsidCapacity] = {};
    uint8_t bssid[BssidLength] = {};

    uint8_t channel = 0;

    WiFiSecurity security =
        WiFiSecurity::Unknown;

    bool hidden = false;

    // Number of successful scans in the measurement session
    // where this BSSID was observed.
    uint8_t seenCount = 0;

    // Signal statistics calculated across the successful scans
    // where the BSSID was present.
    int32_t averageRssi = -127;
    int32_t minimumRssi = -127;
    int32_t maximumRssi = -127;

    WiFiSignalQuality signalQuality =
        WiFiSignalQuality::Unknown;
};

struct WiFiChannelInfo
{
    uint8_t channel = 0;
    uint8_t networkCount = 0;

    int32_t strongestRssi = -127;
    int32_t averageRssi = -127;
};

struct WiFiChannelAssessment
{
    uint8_t channel = 0;

    // Score from the most recently completed scan.
    uint16_t latestScore = 0;

    // Rolling average used for congestion classification
    // and recommendation decisions.
    uint16_t congestionScore = 0;

    WiFiCongestionLevel congestion =
        WiFiCongestionLevel::Unknown;

    bool recommended = false;
    bool comparable = false;
};

struct WiFiChannelRecommendation
{
    uint8_t bestChannel = 0;

    uint16_t bestScore = 0;
    uint16_t secondBestScore = 0;
    uint16_t scoreMargin = 0;

    uint8_t comparableCount = 0;
    uint8_t historySampleCount = 0;

    WiFiRecommendationConfidence confidence =
        WiFiRecommendationConfidence::Unknown;

    bool unique = false;
};

struct WiFiMeasurementSummary
{
    static constexpr uint8_t CandidateCapacity = 3;
    static constexpr uint8_t ChannelCapacity = 15;

    // A measurement session can see more unique BSSIDs than any
    // single scan because APs may appear or disappear between the
    // three successful samples.
    static constexpr uint8_t NetworkCapacity = 128;

    bool available = false;

    uint8_t completedScanCount = 0;

    // Number of networks in the final successful scan. Retained
    // for compatibility with the existing live/session summary.
    uint8_t networkCount = 0;

    // Number of unique BSSIDs observed across the complete
    // measurement session.
    uint8_t observedNetworkCount = 0;

    uint8_t occupiedChannelCount = 0;

    WiFiChannelRecommendation recommendation{};

    WiFiChannelAssessment
        candidates[CandidateCapacity] = {};

    WiFiChannelInfo
        channels[ChannelCapacity] = {};

    WiFiMeasuredNetwork
        networks[NetworkCapacity] = {};
};
