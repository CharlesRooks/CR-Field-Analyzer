#include "WiFiService.h"

WiFiScanState WiFiService::state =
    WiFiScanState::Idle;

WiFiNetworkInfo
    WiFiService::networks[WiFiService::MaxNetworks];

uint8_t WiFiService::networkCount = 0;

void WiFiService::Begin()
{
    ClearResults();
    state = WiFiScanState::Idle;
}

void WiFiService::Update()
{
    // Asynchronous scan processing will be added
    // in Milestone 10.2.
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

void WiFiService::ClearResults()
{
    networkCount = 0;

    for (uint8_t index = 0;
         index < MaxNetworks;
         ++index)
    {
        networks[index] = WiFiNetworkInfo{};
    }
}