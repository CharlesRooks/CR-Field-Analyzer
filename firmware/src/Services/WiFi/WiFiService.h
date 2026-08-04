#pragma once

#include "WiFiTypes.h"

#include <stdint.h>

class WiFiService
{
public:
    static constexpr uint8_t MaxNetworks = 64;
    static constexpr uint8_t Max2_4GHzChannel = 14;
    static constexpr uint8_t CandidateChannelCount = 3;

    // Recommendation decisions use a rolling history
    // of the most recent completed scans.
    static constexpr uint8_t ScoreHistoryDepth = 3;

    // A complete automatic measurement session collects
    // exactly one full rolling-history window.
    static constexpr uint8_t
        AutomaticSessionScanCount = ScoreHistoryDepth;

    // Non-blocking pause between successful automatic scans.
    static constexpr uint32_t
        AutomaticSessionDelayMs = 1500;

    // A transient ESP32 scan failure does not end the
    // measurement session immediately. The same sample is
    // retried and failed attempts are not added to history.
    static constexpr uint8_t
        AutomaticSessionMaxRetries = 2;

    static constexpr uint32_t
        AutomaticSessionRetryDelayMs = 2000;

    // Candidate scores within this many points of the
    // best observed score are treated as comparable.
    static constexpr uint16_t
        ComparableScoreTolerance = 15;

    // A unique winner needs at least this margin over
    // the second-best score for High confidence.
    static constexpr uint16_t
        HighConfidenceMargin = 40;

    static void Begin();
    static void Update();

    static bool StartScan();

    // Starts a fresh measurement session by clearing the
    // current scan cache and rolling score history.
    static bool ResetMeasurementSession();

    // Clears the previous session and automatically collects
    // the three scans required for a mature recommendation.
    static bool StartAutomaticMeasurementSession();

    // Stops the automatic sequence. If a scan is already in
    // progress, it is allowed to finish safely before stopping.
    static bool CancelAutomaticMeasurementSession();

    static WiFiScanState GetState();

    static WiFiMeasurementSessionState
        GetMeasurementSessionState();

    static uint8_t
        GetMeasurementSessionCompletedScanCount();

    static bool IsAutomaticMeasurementSessionActive();
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

    static const WiFiChannelRecommendation &
        GetChannelRecommendation();

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

    static WiFiChannelRecommendation
        channelRecommendation;

    static uint16_t
        candidateScoreHistory
            [CandidateChannelCount]
            [ScoreHistoryDepth];

    static uint8_t scoreHistoryCount;
    static uint8_t scoreHistoryWriteIndex;

    static WiFiMeasurementSessionState
        measurementSessionState;

    static uint8_t
        measurementSessionCompletedScanCount;

    static uint32_t nextAutomaticScanAtMs;
    static uint8_t automaticSessionRetryCount;

    static uint8_t recommendedCandidateIndex;

    static void ClearResults();
    static void ClearChannelStatistics();
    static void ClearChannelAssessments();
    static void ClearScoreHistory();

    static void CopyResults(int16_t resultCount);
    static void SortResultsBySignal();
    static void BuildChannelStatistics();
    static void BuildChannelAssessments();

    static void AddCurrentScoresToHistory();

    static void UpdateAutomaticMeasurementSession();
    static void HandleAutomaticScanCompleted();
    static void HandleAutomaticScanFailed();

    static bool HasReachedDeadline(
        uint32_t nowMs,
        uint32_t deadlineMs);

    static uint16_t CalculateAverageScore(
        uint8_t candidateIndex);

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

    static WiFiRecommendationConfidence
        ClassifyRecommendationConfidence(
            uint8_t comparableCount,
            uint16_t scoreMargin,
            uint8_t historySampleCount);

    static const char *ConfidenceToText(
        WiFiRecommendationConfidence confidence);

    static void PublishScanStarted();
    static void PublishScanCompleted();

    static void LogChannelStatistics();
    static void LogChannelRecommendation();
};
