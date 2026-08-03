#pragma once

#include "WiFiTypes.h"
#include <stdint.h>

class WiFiService
{
public:
    static constexpr uint8_t MaxNetworks = 64;

    static void Begin();
    static void Update();

    static bool StartScan();

    static WiFiScanState GetState();
    static uint8_t GetNetworkCount();

    static constexpr uint8_t Max2_4GHzChannel = 14;

    static uint8_t GetOccupiedChannelCount();

    static const WiFiChannelInfo *GetChannelInfo(
        uint8_t channel);

    static const WiFiNetworkInfo *GetNetwork(
        uint8_t index);

private:
    static WiFiScanState state;

    static WiFiNetworkInfo networks[MaxNetworks];
    static uint8_t networkCount;

    static WiFiChannelInfo
        channelInfo[Max2_4GHzChannel + 1];

    static uint8_t occupiedChannelCount;

    static void ClearResults();
    static void ClearChannelStatistics();
    
    static void CopyResults(int16_t resultCount);
    static void SortResultsBySignal();     
    static void BuildChannelStatistics();
    static void LogChannelStatistics();

    static WiFiSecurity MapSecurity(
        uint8_t encryptionType);

    static WiFiSignalQuality ClassifySignal(
        int32_t rssi);

    static void PublishScanStarted();
    static void PublishScanCompleted();
};