#include "WiFiService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

namespace
{
    // WiFiMeasurementSummary now contains the full AP inventory and is
    // several kilobytes. Keep one static default instance so resets copy
    // from static storage instead of creating a large brace-initialized
    // temporary on the Arduino loop-task stack.
    const WiFiMeasurementSummary EmptyMeasurementSummary{};
}

#include "../../Core/Messaging/MessageBus.h"
#include "../Storage/StorageService.h"

WiFiScanState WiFiService::state =
    WiFiScanState::Idle;

WiFiNetworkInfo
    WiFiService::networks[WiFiService::MaxNetworks];

uint8_t WiFiService::networkCount = 0;

WiFiChannelInfo
    WiFiService::channelInfo[
        WiFiService::Max2_4GHzChannel + 1];

uint8_t WiFiService::occupiedChannelCount = 0;

WiFiChannelAssessment
    WiFiService::candidateAssessments[
        WiFiService::CandidateChannelCount];

WiFiChannelRecommendation
    WiFiService::channelRecommendation{};

WiFiMeasurementSummary
    WiFiService::measurementSummary{};

char WiFiService::measurementSurveyPoint[
    WiFiMeasurementSummary::SurveyPointCapacity] = {};

uint32_t WiFiService::measurementSiteSurveyId = 0;

char WiFiService::measurementSiteSurveyName[
    WiFiMeasurementSummary::SiteSurveyNameCapacity] = {};

WiFiMeasuredNetwork
    WiFiService::measuredNetworks[
        WiFiMeasurementSummary::NetworkCapacity] = {};

int16_t
    WiFiService::measuredNetworkRssiTotals[
        WiFiMeasurementSummary::NetworkCapacity] = {};

uint8_t
    WiFiService::measuredNetworkLastSeenScan[
        WiFiMeasurementSummary::NetworkCapacity] = {};

uint8_t WiFiService::measuredNetworkCount = 0;

uint16_t
    WiFiService::candidateScoreHistory
        [WiFiService::CandidateChannelCount]
        [WiFiService::ScoreHistoryDepth] = {};

uint8_t WiFiService::scoreHistoryCount = 0;
uint8_t WiFiService::scoreHistoryWriteIndex = 0;

WiFiMeasurementSessionState
    WiFiService::measurementSessionState =
        WiFiMeasurementSessionState::Idle;

uint8_t
    WiFiService::measurementSessionCompletedScanCount = 0;

uint32_t WiFiService::nextAutomaticScanAtMs = 0;
uint8_t WiFiService::automaticSessionRetryCount = 0;

uint8_t WiFiService::recommendedCandidateIndex =
    0xFF;

void WiFiService::Begin()
{
    ClearScoreHistory();
    ClearResults();
    ClearMeasurementSummary();
    ClearMeasuredNetworks();

    measurementSessionState =
        WiFiMeasurementSessionState::Idle;

    measurementSessionCompletedScanCount = 0;
    nextAutomaticScanAtMs = 0;
    automaticSessionRetryCount = 0;

    state = WiFiScanState::Idle;

    WiFi.mode(WIFI_STA);

    // Do not erase stored Wi-Fi credentials and do not
    // switch the radio off.
    WiFi.disconnect(false, false);

    WiFi.scanDelete();

    Serial.println("WiFiService: Initialized");
}

bool WiFiService::StartScan()
{
    if (StorageService::IsExternalReadOnlyAccessActive())
    {
        Serial.println(
            "WiFiService: Scan blocked while USB Storage Mode is active");
        return false;
    }

    if (state == WiFiScanState::Scanning)
    {
        return false;
    }

    ClearResults();
    WiFi.scanDelete();

    // Parameters:
    // async = true
    // showHidden = true
    // passive = false
    // maximum milliseconds per channel = 300
    // channel = 0, meaning all channels
    const int16_t result =
        WiFi.scanNetworks(
            true,
            true,
            false,
            300,
            0);

    if (result != WIFI_SCAN_RUNNING)
    {
        state = WiFiScanState::Failed;

        Serial.printf(
            "WiFiService: Failed to start scan (%d)\n",
            result);

        return false;
    }

    state = WiFiScanState::Scanning;

    PublishScanStarted();

    Serial.println("WiFiService: Scan started");

    return true;
}

bool WiFiService::ResetMeasurementSession()
{
    if (StorageService::IsExternalReadOnlyAccessActive())
    {
        Serial.println(
            "WiFiService: Session reset blocked while "
            "USB Storage Mode is active");
        return false;
    }

    // Do not invalidate scan data while the asynchronous
    // radio operation is still writing its results.
    if (state == WiFiScanState::Scanning)
    {
        Serial.println(
            "WiFiService: Session reset rejected "
            "while scan is running");

        return false;
    }

    WiFi.scanDelete();

    ClearScoreHistory();
    ClearResults();
    ClearMeasurementSummary();
    ClearMeasuredNetworks();

    measurementSessionState =
        WiFiMeasurementSessionState::Idle;

    measurementSessionCompletedScanCount = 0;
    nextAutomaticScanAtMs = 0;
    automaticSessionRetryCount = 0;

    state = WiFiScanState::Idle;

    Serial.printf(
        "WiFiService: Measurement session reset, "
        "history 0/%u scans\n",
        ScoreHistoryDepth);

    return true;
}

bool WiFiService::StartAutomaticMeasurementSession()
{
    if (StorageService::IsExternalReadOnlyAccessActive())
    {
        Serial.println(
            "WiFiService: Automatic session blocked while "
            "USB Storage Mode is active");
        return false;
    }

    if (state == WiFiScanState::Scanning ||
        IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    if (!ResetMeasurementSession())
    {
        return false;
    }

    measurementSessionState =
        WiFiMeasurementSessionState::Running;

    Serial.printf(
        "WiFiService: Automatic measurement session "
        "started, target %u scans\n",
        AutomaticSessionScanCount);

    if (!StartScan())
    {
        HandleAutomaticScanFailed();

        return measurementSessionState ==
            WiFiMeasurementSessionState::Running;
    }

    return true;
}

bool WiFiService::CancelAutomaticMeasurementSession()
{
    if (measurementSessionState !=
        WiFiMeasurementSessionState::Running)
    {
        return false;
    }

    if (state == WiFiScanState::Scanning)
    {
        measurementSessionState =
            WiFiMeasurementSessionState::Cancelling;

        Serial.println(
            "WiFiService: Automatic session cancel "
            "requested; finishing active scan");
    }
    else
    {
        measurementSessionState =
            WiFiMeasurementSessionState::Cancelled;

        nextAutomaticScanAtMs = 0;

        Serial.printf(
            "WiFiService: Automatic session cancelled "
            "after %u/%u scans\n",
            measurementSessionCompletedScanCount,
            AutomaticSessionScanCount);
    }

    return true;
}

void WiFiService::Update()
{
    if (state == WiFiScanState::Scanning)
    {
        const int16_t result =
            WiFi.scanComplete();

        if (result == WIFI_SCAN_RUNNING)
        {
            return;
        }

        if (result == WIFI_SCAN_FAILED)
        {
            WiFi.scanDelete();
            state = WiFiScanState::Failed;

            Serial.println("WiFiService: Scan failed");

            HandleAutomaticScanFailed();
            return;
        }

        CopyResults(result);

        // The scan results have now been copied into the
        // SentinelOS fixed-size cache.
        WiFi.scanDelete();

        state = WiFiScanState::Complete;

        Serial.printf(
            "WiFiService: Scan complete, %u networks cached\n",
            networkCount);

        LogChannelStatistics();
        LogChannelRecommendation();

        // Update automatic-session progress before publishing
        // completion so the UI reads the new session state.
        HandleAutomaticScanCompleted();
        PublishScanCompleted();
        return;
    }

    UpdateAutomaticMeasurementSession();
}

WiFiScanState WiFiService::GetState()
{
    return state;
}

WiFiMeasurementSessionState
WiFiService::GetMeasurementSessionState()
{
    return measurementSessionState;
}

uint8_t
WiFiService::GetMeasurementSessionCompletedScanCount()
{
    return measurementSessionCompletedScanCount;
}

bool WiFiService::IsAutomaticMeasurementSessionActive()
{
    return measurementSessionState ==
               WiFiMeasurementSessionState::Running ||
           measurementSessionState ==
               WiFiMeasurementSessionState::Cancelling;
}

uint8_t WiFiService::GetNetworkCount()
{
    return networkCount;
}

const WiFiNetworkInfo *WiFiService::GetNetwork(
    uint8_t index)
{
    if (index >= networkCount)
    {
        return nullptr;
    }

    return &networks[index];
}

uint8_t WiFiService::GetOccupiedChannelCount()
{
    return occupiedChannelCount;
}

const WiFiChannelInfo *WiFiService::GetChannelInfo(
    uint8_t channel)
{
    if (channel == 0 ||
        channel > Max2_4GHzChannel)
    {
        return nullptr;
    }

    return &channelInfo[channel];
}

const WiFiChannelAssessment *
WiFiService::GetCandidateAssessment(
    uint8_t index)
{
    if (index >= CandidateChannelCount)
    {
        return nullptr;
    }

    return &candidateAssessments[index];
}

const WiFiChannelAssessment *
WiFiService::GetRecommendedChannel()
{
    if (recommendedCandidateIndex ==
            InvalidCandidateIndex ||
        recommendedCandidateIndex >=
            CandidateChannelCount)
    {
        return nullptr;
    }

    return &candidateAssessments[
        recommendedCandidateIndex];
}

const WiFiChannelRecommendation &
WiFiService::GetChannelRecommendation()
{
    return channelRecommendation;
}

const WiFiMeasurementSummary &
WiFiService::GetMeasurementSummary()
{
    return measurementSummary;
}

void WiFiService::ClearResults()
{
    networkCount = 0;

    for (uint8_t index = 0;
         index < MaxNetworks;
         ++index)
    {
        networks[index] = WiFiNetworkInfo{};
    }

    ClearChannelStatistics();
    ClearChannelAssessments();
}

void WiFiService::ClearChannelStatistics()
{
    occupiedChannelCount = 0;

    for (uint8_t channel = 0;
         channel <= Max2_4GHzChannel;
         ++channel)
    {
        channelInfo[channel] =
            WiFiChannelInfo{};

        channelInfo[channel].channel =
            channel;
    }
}

void WiFiService::ClearChannelAssessments()
{
    static constexpr uint8_t
        candidateChannels[
            CandidateChannelCount] =
        {
            1,
            6,
            11
        };

    recommendedCandidateIndex =
        InvalidCandidateIndex;

    channelRecommendation =
        WiFiChannelRecommendation{};

    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        candidateAssessments[index] =
            WiFiChannelAssessment{};

        candidateAssessments[index].channel =
            candidateChannels[index];
    }
}

void WiFiService::ClearScoreHistory()
{
    scoreHistoryCount = 0;
    scoreHistoryWriteIndex = 0;

    for (uint8_t candidateIndex = 0;
         candidateIndex < CandidateChannelCount;
         ++candidateIndex)
    {
        for (uint8_t sampleIndex = 0;
             sampleIndex < ScoreHistoryDepth;
             ++sampleIndex)
        {
            candidateScoreHistory
                [candidateIndex]
                [sampleIndex] = 0;
        }
    }
}

void WiFiService::ClearMeasurementSummary()
{
    measurementSummary =
        EmptyMeasurementSummary;
}

void WiFiService::ClearMeasuredNetworks()
{
    measuredNetworkCount = 0;

    for (uint8_t index = 0;
         index < WiFiMeasurementSummary::NetworkCapacity;
         ++index)
    {
        measuredNetworks[index] =
            WiFiMeasuredNetwork{};

        measuredNetworkRssiTotals[index] = 0;
        measuredNetworkLastSeenScan[index] = 0;
    }
}

void WiFiService::AccumulateMeasuredNetworks()
{
    const uint8_t scanOrdinal =
        measurementSessionCompletedScanCount + 1;

    for (uint8_t networkIndex = 0;
         networkIndex < networkCount;
         ++networkIndex)
    {
        const WiFiNetworkInfo &network =
            networks[networkIndex];

        int16_t matchedIndex = -1;

        for (uint8_t measuredIndex = 0;
             measuredIndex < measuredNetworkCount;
             ++measuredIndex)
        {
            if (std::memcmp(
                    measuredNetworks[measuredIndex].bssid,
                    network.bssid,
                    WiFiNetworkInfo::BssidLength) == 0)
            {
                matchedIndex =
                    static_cast<int16_t>(measuredIndex);
                break;
            }
        }

        if (matchedIndex < 0)
        {
            if (measuredNetworkCount >=
                WiFiMeasurementSummary::NetworkCapacity)
            {
                Serial.println(
                    "WiFiService: Measurement network inventory "
                    "capacity reached; additional BSSIDs omitted");
                break;
            }

            matchedIndex =
                static_cast<int16_t>(
                    measuredNetworkCount);

            WiFiMeasuredNetwork &measured =
                measuredNetworks[measuredNetworkCount];

            std::strncpy(
                measured.ssid,
                network.ssid,
                WiFiMeasuredNetwork::SsidCapacity - 1);

            measured.ssid[
                WiFiMeasuredNetwork::SsidCapacity - 1] = '\0';

            std::memcpy(
                measured.bssid,
                network.bssid,
                WiFiMeasuredNetwork::BssidLength);

            measured.channel = network.channel;
            measured.security = network.security;
            measured.hidden = network.hidden;
            measured.seenCount = 0;
            measured.averageRssi = network.rssi;
            measured.minimumRssi = network.rssi;
            measured.maximumRssi = network.rssi;
            measured.signalQuality = network.signalQuality;

            measuredNetworkRssiTotals[
                measuredNetworkCount] = 0;

            measuredNetworkLastSeenScan[
                measuredNetworkCount] = 0;

            ++measuredNetworkCount;
        }

        const uint8_t index =
            static_cast<uint8_t>(matchedIndex);

        // Count a BSSID at most once per successful scan.
        if (measuredNetworkLastSeenScan[index] ==
            scanOrdinal)
        {
            continue;
        }

        WiFiMeasuredNetwork &measured =
            measuredNetworks[index];

        if (network.ssid[0] != '\0')
        {
            std::strncpy(
                measured.ssid,
                network.ssid,
                WiFiMeasuredNetwork::SsidCapacity - 1);

            measured.ssid[
                WiFiMeasuredNetwork::SsidCapacity - 1] = '\0';

            measured.hidden = false;
        }

        if (network.channel != 0)
        {
            measured.channel = network.channel;
        }

        measured.security = network.security;

        if (measured.seenCount == 0)
        {
            measured.minimumRssi = network.rssi;
            measured.maximumRssi = network.rssi;
        }
        else
        {
            if (network.rssi < measured.minimumRssi)
            {
                measured.minimumRssi = network.rssi;
            }

            if (network.rssi > measured.maximumRssi)
            {
                measured.maximumRssi = network.rssi;
            }
        }

        measuredNetworkRssiTotals[index] +=
            static_cast<int16_t>(network.rssi);

        if (measured.seenCount < 255)
        {
            ++measured.seenCount;
        }

        measuredNetworkLastSeenScan[index] =
            scanOrdinal;
    }

    Serial.printf(
        "WiFiService: Measurement network inventory "
        "%u unique BSSID%s after sample %u/%u\n",
        measuredNetworkCount,
        measuredNetworkCount == 1 ? "" : "s",
        scanOrdinal,
        AutomaticSessionScanCount);
}

void WiFiService::CaptureMeasurementSummary()
{
    measurementSummary =
        EmptyMeasurementSummary;

    measurementSummary.available = true;

    measurementSummary.siteSurveyId =
    measurementSiteSurveyId;

    std::strncpy(
        measurementSummary.siteSurveyName,
        measurementSiteSurveyName,
        WiFiMeasurementSummary::
            SiteSurveyNameCapacity - 1);

    measurementSummary.siteSurveyName[
        WiFiMeasurementSummary::
            SiteSurveyNameCapacity - 1] = '\0';

    std::strncpy(
        measurementSummary.surveyPoint,
        measurementSurveyPoint,
        WiFiMeasurementSummary::
            SurveyPointCapacity - 1);

    measurementSummary.surveyPoint[
        WiFiMeasurementSummary::
            SurveyPointCapacity - 1] = '\0';

    measurementSummary.completedScanCount =
        measurementSessionCompletedScanCount;

    measurementSummary.networkCount =
        networkCount;

    measurementSummary.occupiedChannelCount =
        occupiedChannelCount;

    measurementSummary.recommendation =
        channelRecommendation;

    for (uint8_t index = 0;
         index < CandidateChannelCount &&
         index <
             WiFiMeasurementSummary::
                 CandidateCapacity;
         ++index)
    {
        measurementSummary.candidates[index] =
            candidateAssessments[index];
    }

    for (uint8_t channel = 0;
         channel <= Max2_4GHzChannel &&
         channel <
             WiFiMeasurementSummary::
                 ChannelCapacity;
         ++channel)
    {
        measurementSummary.channels[channel] =
            channelInfo[channel];
    }

    measurementSummary.observedNetworkCount =
        measuredNetworkCount;

    for (uint8_t index = 0;
         index < measuredNetworkCount;
         ++index)
    {
        WiFiMeasuredNetwork measured =
            measuredNetworks[index];

        if (measured.seenCount > 0)
        {
            measured.averageRssi =
                measuredNetworkRssiTotals[index] /
                measured.seenCount;

            measured.signalQuality =
                ClassifySignal(
                    measured.averageRssi);
        }

        measurementSummary.networks[index] =
            measured;
    }

    Serial.printf(
        "WiFiService: Session summary captured - "
        "%u networks in final scan, %u unique BSSIDs, "
        "%u occupied channels, best CH %u, confidence %s\n",
        measurementSummary.networkCount,
        measurementSummary.observedNetworkCount,
        measurementSummary.occupiedChannelCount,
        measurementSummary.recommendation.bestChannel,
        ConfidenceToText(
            measurementSummary.recommendation
                .confidence));

    // The completed summary owns its Survey Point label from this
    // point onward. Clear the working label so the next physical
    // survey location must be identified deliberately.
    measurementSurveyPoint[0] = '\0';
}

void WiFiService::CopyResults(
    int16_t resultCount)
{
    ClearResults();

    if (resultCount <= 0)
    {
        BuildChannelAssessments();
        return;
    }

    const uint16_t availableCount =
        static_cast<uint16_t>(resultCount);

    const uint8_t copyCount =
        availableCount > MaxNetworks
            ? MaxNetworks
            : static_cast<uint8_t>(availableCount);

    for (uint8_t sourceIndex = 0;
         sourceIndex < copyCount;
         ++sourceIndex)
    {
        String ssid;
        uint8_t encryptionType = 0;
        int32_t rssi = -127;
        uint8_t *bssid = nullptr;
        int32_t channel = 0;

        if (!WiFi.getNetworkInfo(
                sourceIndex,
                ssid,
                encryptionType,
                rssi,
                bssid,
                channel))
        {
            continue;
        }

        WiFiNetworkInfo &network =
            networks[networkCount];

        std::strncpy(
            network.ssid,
            ssid.c_str(),
            WiFiNetworkInfo::SsidCapacity - 1);

        network.ssid[
            WiFiNetworkInfo::SsidCapacity - 1] = '\0';

        if (bssid != nullptr)
        {
            std::memcpy(
                network.bssid,
                bssid,
                WiFiNetworkInfo::BssidLength);
        }

        network.rssi = rssi;

        network.signalQuality =
            ClassifySignal(rssi);

        network.channel =
            channel > 0 && channel <= 255
                ? static_cast<uint8_t>(channel)
                : 0;

        network.security =
            MapSecurity(encryptionType);

        network.hidden =
            ssid.length() == 0;

        ++networkCount;
    }

    // Perform aggregate work once after all scan results
    // have been copied into the fixed cache.
    SortResultsBySignal();
    BuildChannelStatistics();
    BuildChannelAssessments();
}

void WiFiService::SortResultsBySignal()
{
    if (networkCount < 2)
    {
        return;
    }

    for (uint8_t index = 1;
         index < networkCount;
         ++index)
    {
        const WiFiNetworkInfo current =
            networks[index];

        int16_t compareIndex =
            static_cast<int16_t>(index) - 1;

        while (compareIndex >= 0 &&
               networks[compareIndex].rssi <
                   current.rssi)
        {
            networks[compareIndex + 1] =
                networks[compareIndex];

            --compareIndex;
        }

        networks[compareIndex + 1] =
            current;
    }
}

void WiFiService::BuildChannelStatistics()
{
    ClearChannelStatistics();

    int32_t rssiTotals[
        Max2_4GHzChannel + 1] = {};

    for (uint8_t index = 0;
         index < networkCount;
         ++index)
    {
        const WiFiNetworkInfo &network =
            networks[index];

        const uint8_t channel =
            network.channel;

        if (channel == 0 ||
            channel > Max2_4GHzChannel)
        {
            continue;
        }

        WiFiChannelInfo &info =
            channelInfo[channel];

        if (info.networkCount == 0)
        {
            ++occupiedChannelCount;

            info.strongestRssi =
                network.rssi;
        }
        else if (network.rssi >
                 info.strongestRssi)
        {
            info.strongestRssi =
                network.rssi;
        }

        ++info.networkCount;

        rssiTotals[channel] +=
            network.rssi;
    }

    for (uint8_t channel = 1;
         channel <= Max2_4GHzChannel;
         ++channel)
    {
        WiFiChannelInfo &info =
            channelInfo[channel];

        if (info.networkCount == 0)
        {
            continue;
        }

        info.averageRssi =
            rssiTotals[channel] /
            info.networkCount;
    }
}

void WiFiService::BuildChannelAssessments()
{
    ClearChannelAssessments();

    // A zero-network result should not be averaged with
    // measurements from a previous RF environment.
    if (networkCount == 0)
    {
        ClearScoreHistory();
    }

    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        WiFiChannelAssessment &assessment =
            candidateAssessments[index];

        assessment.latestScore =
            CalculateCongestionScore(
                assessment.channel);
    }

    AddCurrentScoresToHistory();

    uint16_t bestScore = 0xFFFF;
    uint16_t secondBestScore = 0xFFFF;

    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        WiFiChannelAssessment &assessment =
            candidateAssessments[index];

        assessment.congestionScore =
            CalculateAverageScore(index);

        assessment.congestion =
            ClassifyCongestion(
                assessment.congestionScore);

        if (assessment.congestionScore <
            bestScore)
        {
            secondBestScore = bestScore;
            bestScore =
                assessment.congestionScore;
            recommendedCandidateIndex =
                index;
        }
        else if (assessment.congestionScore <
                 secondBestScore)
        {
            secondBestScore =
                assessment.congestionScore;
        }
    }

    if (recommendedCandidateIndex ==
            InvalidCandidateIndex)
    {
        return;
    }

    WiFiChannelAssessment &bestAssessment =
        candidateAssessments[
            recommendedCandidateIndex];

    bestAssessment.recommended = true;

    uint8_t comparableCount = 0;

    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        WiFiChannelAssessment &assessment =
            candidateAssessments[index];

        const uint32_t comparableLimit =
            static_cast<uint32_t>(bestScore) +
            ComparableScoreTolerance;

        if (assessment.congestionScore <=
            comparableLimit)
        {
            assessment.comparable = true;
            ++comparableCount;
        }
    }

    const uint16_t scoreMargin =
        secondBestScore >= bestScore
            ? secondBestScore - bestScore
            : 0;

    channelRecommendation.bestChannel =
        bestAssessment.channel;
    channelRecommendation.bestScore =
        bestScore;
    channelRecommendation.secondBestScore =
        secondBestScore;
    channelRecommendation.scoreMargin =
        scoreMargin;
    channelRecommendation.comparableCount =
        comparableCount;
    channelRecommendation.historySampleCount =
        scoreHistoryCount;
    channelRecommendation.unique =
        comparableCount == 1;
    channelRecommendation.confidence =
        networkCount == 0
            ? WiFiRecommendationConfidence::Unknown
            : ClassifyRecommendationConfidence(
                  comparableCount,
                  scoreMargin,
                  scoreHistoryCount);
}

void WiFiService::AddCurrentScoresToHistory()
{
    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        candidateScoreHistory
            [index]
            [scoreHistoryWriteIndex] =
                candidateAssessments[index]
                    .latestScore;
    }

    scoreHistoryWriteIndex =
        static_cast<uint8_t>(
            (scoreHistoryWriteIndex + 1) %
            ScoreHistoryDepth);

    if (scoreHistoryCount <
        ScoreHistoryDepth)
    {
        ++scoreHistoryCount;
    }
}

void WiFiService::UpdateAutomaticMeasurementSession()
{
    if (measurementSessionState !=
        WiFiMeasurementSessionState::Running)
    {
        return;
    }

    if (measurementSessionCompletedScanCount >=
        AutomaticSessionScanCount)
    {
        measurementSessionState =
            WiFiMeasurementSessionState::Complete;
        return;
    }

    if (!HasReachedDeadline(
            millis(),
            nextAutomaticScanAtMs))
    {
        return;
    }

    if (!StartScan())
    {
        HandleAutomaticScanFailed();
    }
}

void WiFiService::HandleAutomaticScanCompleted()
{
    if (measurementSessionState !=
            WiFiMeasurementSessionState::Running &&
        measurementSessionState !=
            WiFiMeasurementSessionState::Cancelling)
    {
        return;
    }

    // A successful sample clears the consecutive retry count.
    automaticSessionRetryCount = 0;

    // Capture the AP/BSSID inventory before the live result cache is
    // replaced by the next scan.
    AccumulateMeasuredNetworks();

    if (measurementSessionCompletedScanCount <
        AutomaticSessionScanCount)
    {
        ++measurementSessionCompletedScanCount;
    }

    if (measurementSessionState ==
        WiFiMeasurementSessionState::Cancelling)
    {
        measurementSessionState =
            WiFiMeasurementSessionState::Cancelled;

        nextAutomaticScanAtMs = 0;

        Serial.printf(
            "WiFiService: Automatic session cancelled "
            "after %u/%u scans\n",
            measurementSessionCompletedScanCount,
            AutomaticSessionScanCount);

        return;
    }

    if (measurementSessionCompletedScanCount >=
        AutomaticSessionScanCount)
    {
        CaptureMeasurementSummary();

        measurementSessionState =
            WiFiMeasurementSessionState::Complete;

        nextAutomaticScanAtMs = 0;

        PublishMeasurementSessionCompleted();

        Serial.printf(
            "WiFiService: Automatic measurement session "
            "complete, %u/%u scans\n",
            measurementSessionCompletedScanCount,
            AutomaticSessionScanCount);

        return;
    }

    nextAutomaticScanAtMs =
        millis() + AutomaticSessionDelayMs;

    Serial.printf(
        "WiFiService: Automatic session progress "
        "%u/%u; next scan in %lu ms\n",
        measurementSessionCompletedScanCount,
        AutomaticSessionScanCount,
        static_cast<unsigned long>(
            AutomaticSessionDelayMs));
}

void WiFiService::HandleAutomaticScanFailed()
{
    if (!IsAutomaticMeasurementSessionActive())
    {
        return;
    }

    if (measurementSessionState ==
        WiFiMeasurementSessionState::Cancelling)
    {
        measurementSessionState =
            WiFiMeasurementSessionState::Cancelled;

        nextAutomaticScanAtMs = 0;
        automaticSessionRetryCount = 0;

        Serial.printf(
            "WiFiService: Automatic session cancelled "
            "after %u/%u completed scans\n",
            measurementSessionCompletedScanCount,
            AutomaticSessionScanCount);

        return;
    }

    if (automaticSessionRetryCount <
        AutomaticSessionMaxRetries)
    {
        ++automaticSessionRetryCount;

        measurementSessionState =
            WiFiMeasurementSessionState::Running;

        nextAutomaticScanAtMs =
            millis() +
            AutomaticSessionRetryDelayMs;

        Serial.printf(
            "WiFiService: Automatic scan retry %u/%u "
            "scheduled in %lu ms; progress remains "
            "%u/%u completed scans\n",
            automaticSessionRetryCount,
            AutomaticSessionMaxRetries,
            static_cast<unsigned long>(
                AutomaticSessionRetryDelayMs),
            measurementSessionCompletedScanCount,
            AutomaticSessionScanCount);

        return;
    }

    measurementSessionState =
        WiFiMeasurementSessionState::Failed;

    nextAutomaticScanAtMs = 0;

    Serial.printf(
        "WiFiService: Automatic session failed "
        "after %u retries and %u/%u completed scans\n",
        AutomaticSessionMaxRetries,
        measurementSessionCompletedScanCount,
        AutomaticSessionScanCount);
}

bool WiFiService::HasReachedDeadline(
    uint32_t nowMs,
    uint32_t deadlineMs)
{
    return static_cast<int32_t>(
               nowMs - deadlineMs) >= 0;
}

bool WiFiService::SetMeasurementSurveyPoint(
    const char *surveyPoint)
{
    if (surveyPoint == nullptr ||
        state == WiFiScanState::Scanning ||
        IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    std::strncpy(
        measurementSurveyPoint,
        surveyPoint,
        WiFiMeasurementSummary::
            SurveyPointCapacity - 1);

    measurementSurveyPoint[
        WiFiMeasurementSummary::
            SurveyPointCapacity - 1] = '\0';

    return true;
}

bool WiFiService::SetMeasurementSiteSurvey(
    uint32_t surveyId,
    const char *surveyName)
{
    if (state == WiFiScanState::Scanning ||
        IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    if (surveyId == 0)
    {
        measurementSiteSurveyId = 0;
        measurementSiteSurveyName[0] = '\0';
        return true;
    }

    if (surveyName == nullptr ||
        surveyName[0] == '\0')
    {
        return false;
    }

    measurementSiteSurveyId = surveyId;

    std::strncpy(
        measurementSiteSurveyName,
        surveyName,
        WiFiMeasurementSummary::
            SiteSurveyNameCapacity - 1);

    measurementSiteSurveyName[
        WiFiMeasurementSummary::
            SiteSurveyNameCapacity - 1] = '\0';

    return true;
}

const char *WiFiService::GetMeasurementSurveyPoint()
{
    return measurementSurveyPoint;
}

uint16_t WiFiService::CalculateAverageScore(
    uint8_t candidateIndex)
{
    if (candidateIndex >=
            CandidateChannelCount ||
        scoreHistoryCount == 0)
    {
        return 0;
    }

    uint32_t total = 0;

    for (uint8_t sampleIndex = 0;
         sampleIndex < scoreHistoryCount;
         ++sampleIndex)
    {
        total += candidateScoreHistory
            [candidateIndex]
            [sampleIndex];
    }

    return static_cast<uint16_t>(
        (total + scoreHistoryCount / 2) /
        scoreHistoryCount);
}

uint16_t WiFiService::CalculateCongestionScore(
    uint8_t candidateChannel)
{
    uint32_t score = 0;

    for (uint8_t index = 0;
         index < networkCount;
         ++index)
    {
        const WiFiNetworkInfo &network =
            networks[index];

        if (network.channel == 0 ||
            network.channel >
                Max2_4GHzChannel)
        {
            continue;
        }

        const uint8_t overlapWeight =
            CalculateOverlapWeight(
                candidateChannel,
                network.channel);

        if (overlapWeight == 0)
        {
            continue;
        }

        const uint8_t signalImpact =
            CalculateSignalImpact(
                network.rssi);

        const uint16_t contribution =
            static_cast<uint16_t>(
                (static_cast<uint16_t>(
                    signalImpact) *
                 overlapWeight +
                 50) /
                100);

        score += contribution;
    }

    if (score > 0xFFFF)
    {
        return 0xFFFF;
    }

    return static_cast<uint16_t>(score);
}

uint8_t WiFiService::CalculateSignalImpact(
    int32_t rssi)
{
    if (rssi >= -50)
    {
        return 100;
    }

    if (rssi >= -60)
    {
        return 85;
    }

    if (rssi >= -70)
    {
        return 65;
    }

    if (rssi >= -80)
    {
        return 40;
    }

    if (rssi >= -90)
    {
        return 20;
    }

    return 8;
}

uint8_t WiFiService::CalculateOverlapWeight(
    uint8_t candidateChannel,
    uint8_t networkChannel)
{
    const uint8_t distance =
        candidateChannel > networkChannel
            ? candidateChannel -
                networkChannel
            : networkChannel -
                candidateChannel;

    switch (distance)
    {
        case 0:
            return 100;

        case 1:
            return 80;

        case 2:
            return 55;

        case 3:
            return 30;

        case 4:
            return 10;

        default:
            return 0;
    }
}

WiFiSecurity WiFiService::MapSecurity(
    uint8_t encryptionType)
{
    switch (encryptionType)
    {
        case WIFI_AUTH_OPEN:
            return WiFiSecurity::Open;

        case WIFI_AUTH_WEP:
            return WiFiSecurity::WEP;

        case WIFI_AUTH_WPA_PSK:
            return WiFiSecurity::WPA_PSK;

        case WIFI_AUTH_WPA2_PSK:
            return WiFiSecurity::WPA2_PSK;

        case WIFI_AUTH_WPA_WPA2_PSK:
            return WiFiSecurity::WPA_WPA2_PSK;

        case WIFI_AUTH_ENTERPRISE:
            return WiFiSecurity::WPA2_Enterprise;

        case WIFI_AUTH_WPA3_PSK:
            return WiFiSecurity::WPA3_PSK;

        case WIFI_AUTH_WPA2_WPA3_PSK:
            return WiFiSecurity::WPA2_WPA3_PSK;

        case WIFI_AUTH_WAPI_PSK:
            return WiFiSecurity::WAPI_PSK;

        default:
            return WiFiSecurity::Unknown;
    }
}

WiFiSignalQuality WiFiService::ClassifySignal(
    int32_t rssi)
{
    if (rssi >= -55)
    {
        return WiFiSignalQuality::Excellent;
    }

    if (rssi >= -67)
    {
        return WiFiSignalQuality::Good;
    }

    if (rssi >= -75)
    {
        return WiFiSignalQuality::Fair;
    }

    return WiFiSignalQuality::Poor;
}

WiFiCongestionLevel WiFiService::ClassifyCongestion(
    uint16_t score)
{
    if (score <= 60)
    {
        return WiFiCongestionLevel::Excellent;
    }

    if (score <= 140)
    {
        return WiFiCongestionLevel::Good;
    }

    if (score <= 240)
    {
        return WiFiCongestionLevel::Fair;
    }

    return WiFiCongestionLevel::Poor;
}

WiFiRecommendationConfidence
WiFiService::ClassifyRecommendationConfidence(
    uint8_t comparableCount,
    uint16_t scoreMargin,
    uint8_t historySampleCount)
{
    if (comparableCount > 1 ||
        historySampleCount < 2)
    {
        return WiFiRecommendationConfidence::Low;
    }

    if (historySampleCount >=
            ScoreHistoryDepth &&
        scoreMargin >=
            HighConfidenceMargin)
    {
        return WiFiRecommendationConfidence::High;
    }

    return WiFiRecommendationConfidence::Medium;
}

const char *WiFiService::ConfidenceToText(
    WiFiRecommendationConfidence confidence)
{
    switch (confidence)
    {
        case WiFiRecommendationConfidence::High:
            return "High";
        case WiFiRecommendationConfidence::Medium:
            return "Medium";
        case WiFiRecommendationConfidence::Low:
            return "Low";
        case WiFiRecommendationConfidence::Unknown:
        default:
            return "Unknown";
    }
}

void WiFiService::PublishScanStarted()
{
    Message message{};
    message.type = MessageType::WiFiScanStarted;
    message.timestampMs = millis();

    MessageBus::Publish(message);
}

void WiFiService::PublishScanCompleted()
{
    Message message{};
    message.type = MessageType::WiFiScanCompleted;
    message.timestampMs = millis();
    message.wifiNetworkCount = networkCount;

    MessageBus::Publish(message);
}

void WiFiService::PublishMeasurementSessionCompleted()
{
    Message message{};
    message.type =
        MessageType::WiFiMeasurementSessionCompleted;
    message.timestampMs = millis();

    MessageBus::Publish(message);
}

void WiFiService::LogChannelStatistics()
{
    Serial.printf(
        "WiFiService: %u occupied channels\n",
        occupiedChannelCount);

    for (uint8_t channel = 1;
         channel <= Max2_4GHzChannel;
         ++channel)
    {
        const WiFiChannelInfo &info =
            channelInfo[channel];

        if (info.networkCount == 0)
        {
            continue;
        }

        Serial.printf(
            "WiFiService: CH %u - "
            "%u network%s, strongest %ld dBm, "
            "average %ld dBm\n",
            info.channel,
            info.networkCount,
            info.networkCount == 1 ? "" : "s",
            static_cast<long>(
                info.strongestRssi),
            static_cast<long>(
                info.averageRssi));
    }
}

void WiFiService::LogChannelRecommendation()
{
    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        const WiFiChannelAssessment &assessment =
            candidateAssessments[index];

        Serial.printf(
            "WiFiService: Candidate CH %u - "
            "latest %u, average %u over %u scan%s"
            "%s%s\n",
            assessment.channel,
            assessment.latestScore,
            assessment.congestionScore,
            scoreHistoryCount,
            scoreHistoryCount == 1 ? "" : "s",
            assessment.recommended
                ? ", best averaged"
                : "",
            assessment.comparable &&
                    !assessment.recommended
                ? ", comparable"
                : "");
    }

    if (recommendedCandidateIndex ==
        InvalidCandidateIndex)
    {
        Serial.println(
            "WiFiService: No channel recommendation");
        return;
    }

    Serial.printf(
        "WiFiService: Best averaged CH %u, "
        "score %u, margin %u, confidence %s, "
        "%u comparable candidate%s, "
        "history %u/%u scans\n",
        channelRecommendation.bestChannel,
        channelRecommendation.bestScore,
        channelRecommendation.scoreMargin,
        ConfidenceToText(
            channelRecommendation.confidence),
        channelRecommendation.comparableCount,
        channelRecommendation.comparableCount == 1
            ? ""
            : "s",
        channelRecommendation.historySampleCount,
        ScoreHistoryDepth);
}
