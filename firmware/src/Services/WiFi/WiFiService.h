#pragma once

#include "WiFiTypes.h"
#include <stdint.h>

class WiFiService
{
public:
    static constexpr uint8_t MaxNetworks = 64;

    static void Begin();
    static void Update();

    static WiFiScanState GetState();

    static uint8_t GetNetworkCount();

    static const WiFiNetworkInfo *GetNetwork(
        uint8_t index);

private:
    static WiFiScanState state;

    static WiFiNetworkInfo networks[MaxNetworks];
    static uint8_t networkCount;

    static void ClearResults();
};