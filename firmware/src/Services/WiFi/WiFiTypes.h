#pragma once

#include <stdint.h>

enum class WiFiScanState : uint8_t
{
    Idle = 0,
    Scanning,
    Complete,
    Failed
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

    WiFiSecurity security = WiFiSecurity::Unknown;

    bool hidden = false;
};