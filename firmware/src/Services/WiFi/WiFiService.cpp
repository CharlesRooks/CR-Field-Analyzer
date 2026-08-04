#include "WiFiService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

#include "../../Core/Messaging/MessageBus.h"

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

uint8_t WiFiService::recommendedCandidateIndex =
    0xFF;

void WiFiService::Begin()
{
    ClearResults();
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

void WiFiService::Update()
{
    if (state != WiFiScanState::Scanning)
    {
        return;
    }

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

        return;
    }

    CopyResults(result);

    // The scan results have now been copied into the
    // SentinelOS fixed-size cache.
    WiFi.scanDelete();

    state = WiFiScanState::Complete;

    PublishScanCompleted();

    Serial.printf(
        "WiFiService: Scan complete, %u networks cached\n",
        networkCount);

    LogChannelStatistics();
    LogChannelRecommendation();
}

WiFiScanState WiFiService::GetState()
{
    return state;
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

    uint16_t bestScore = 0xFFFF;
    uint16_t secondBestScore = 0xFFFF;

    for (uint8_t index = 0;
         index < CandidateChannelCount;
         ++index)
    {
        WiFiChannelAssessment &assessment =
            candidateAssessments[index];

        assessment.congestionScore =
            CalculateCongestionScore(
                assessment.channel);

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
    channelRecommendation.unique =
        comparableCount == 1;
    channelRecommendation.confidence =
        networkCount == 0
            ? WiFiRecommendationConfidence::Unknown
            : ClassifyRecommendationConfidence(
                  comparableCount,
                  scoreMargin);
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
    uint16_t scoreMargin)
{
    if (comparableCount > 1)
    {
        return WiFiRecommendationConfidence::Low;
    }

    if (scoreMargin >=
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
            "score %u%s%s\n",
            assessment.channel,
            assessment.congestionScore,
            assessment.recommended
                ? ", best observed"
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
        "WiFiService: Best observed CH %u, "
        "score %u, margin %u, confidence %s, "
        "%u comparable candidate%s\n",
        channelRecommendation.bestChannel,
        channelRecommendation.bestScore,
        channelRecommendation.scoreMargin,
        ConfidenceToText(
            channelRecommendation.confidence),
        channelRecommendation.comparableCount,
        channelRecommendation.comparableCount == 1
            ? ""
            : "s");
}
