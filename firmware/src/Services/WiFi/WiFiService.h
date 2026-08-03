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

    static const WiFiNetworkInfo *GetNetwork(
        uint8_t index);

private:
    static WiFiScanState state;

    static WiFiNetworkInfo networks[MaxNetworks];
    static uint8_t networkCount;

    static void ClearResults();
    static void CopyResults(int16_t resultCount);

    static WiFiSecurity MapSecurity(
        uint8_t encryptionType);

    static void PublishScanStarted();
    static void PublishScanCompleted();
};