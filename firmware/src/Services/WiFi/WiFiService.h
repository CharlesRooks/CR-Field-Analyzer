#pragma once

#include "WiFiTypes.h"

#include <stdint.h>

class WiFiService
{
public:
    static constexpr uint8_t MaxNetworks = 64;
    static constexpr uint8_t Max2_4GHzChannel = 14;
    static constexpr uint8_t CandidateChannelCount = 3;

    static void Begin();
    static void Update();

    static bool StartScan();

    static WiFiScanState GetState();
    static uint8_t GetNetworkCount();

    static const WiFiNetworkInfo *GetNetwork(
        uint8_t index);

    static uint8_t GetOccupiedChannelCount();

    static const WiFiChannelInfo *GetChannelInfo(
        uint8_t channel);

    static const WiFiChannelAssessment *
        GetCandidateAssessment(
            uint8_t index);

    static const WiFiChannelAssessment *
        GetRecommendedChannel();

private:
    static constexpr uint8_t
        InvalidCandidateIndex = 0xFF;

    static WiFiScanState state;

    static WiFiNetworkInfo networks[MaxNetworks];
    static uint8_t networkCount;

    static WiFiChannelInfo
        channelInfo[Max2_4GHzChannel + 1];

    static uint8_t occupiedChannelCount;

    static WiFiChannelAssessment
        candidateAssessments[CandidateChannelCount];

    static uint8_t recommendedCandidateIndex;

    static void ClearResults();
    static void ClearChannelStatistics();
    static void ClearChannelAssessments();

    static void CopyResults(int16_t resultCount);
    static void SortResultsBySignal();
    static void BuildChannelStatistics();
    static void BuildChannelAssessments();

    static uint16_t CalculateCongestionScore(
        uint8_t candidateChannel);

    static uint8_t CalculateSignalImpact(
        int32_t rssi);

    static uint8_t CalculateOverlapWeight(
        uint8_t candidateChannel,
        uint8_t networkChannel);

    static WiFiSecurity MapSecurity(
        uint8_t encryptionType);

    static WiFiSignalQuality ClassifySignal(
        int32_t rssi);

    static WiFiCongestionLevel ClassifyCongestion(
        uint16_t score);

    static void PublishScanStarted();
    static void PublishScanCompleted();

    static void LogChannelStatistics();
    static void LogChannelRecommendation();
};
