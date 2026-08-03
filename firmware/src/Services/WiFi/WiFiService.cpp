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

void WiFiService::CopyResults(
    int16_t resultCount)
{
    ClearResults();

    if (resultCount <= 0)
    {
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

        SortResultsBySignal();
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