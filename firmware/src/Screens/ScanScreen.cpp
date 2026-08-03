#include "ScanScreen.h"

#include "../Core/Messaging/MessageBus.h"
#include "../Services/WiFi/WiFiService.h"
#include "../UI/Theme.h"

#include <Arduino.h>
#include <cstdio>

ScanScreen *ScanScreen::instance = nullptr;

void ScanScreen::Begin()
{
    instance = this;

    if (!MessageBus::Subscribe(
            MessageType::WiFiScanStarted,
            ScanScreen::HandleMessage))
    {
        Serial.println(
            "ScanScreen: Failed to subscribe "
            "to WiFiScanStarted");
    }

    if (!MessageBus::Subscribe(
            MessageType::WiFiScanCompleted,
            ScanScreen::HandleMessage))
    {
        Serial.println(
            "ScanScreen: Failed to subscribe "
            "to WiFiScanCompleted");
    }
}

void ScanScreen::CreateContent()
{
    lv_obj_t *parent = GetContentArea();

    lv_obj_t *layout =
        lv_obj_create(parent);

    lv_obj_remove_style_all(layout);

    lv_obj_set_size(
        layout,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        layout,
        8,
        0);

    lv_obj_set_style_pad_row(
        layout,
        8,
        0);

    lv_obj_set_flex_flow(
        layout,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        layout,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header =
        lv_obj_create(layout);

    lv_obj_remove_style_all(header);

    lv_obj_set_width(
        header,
        lv_pct(100));

    lv_obj_set_height(
        header,
        32);

    lv_obj_set_flex_flow(
        header,
        LV_FLEX_FLOW_ROW);

    lv_obj_set_flex_align(
        header,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title =
        lv_label_create(header);

    lv_label_set_text(
        title,
        "Wi-Fi Scan");

    lv_obj_set_style_text_color(
        title,
        Theme::Accent(),
        0);

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0);

    statusLabel =
        lv_label_create(header);

    lv_label_set_text(
        statusLabel,
        "Loading");

    lv_obj_set_style_text_color(
        statusLabel,
        Theme::Muted(),
        0);

    networkList =
        lv_obj_create(layout);

    lv_obj_set_width(
        networkList,
        lv_pct(100));

    lv_obj_set_flex_grow(
        networkList,
        1);

    lv_obj_set_flex_flow(
        networkList,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_bg_opa(
        networkList,
        LV_OPA_TRANSP,
        0);

    lv_obj_set_style_border_width(
        networkList,
        0,
        0);

    lv_obj_set_style_pad_all(
        networkList,
        4,
        0);

    lv_obj_set_style_pad_row(
        networkList,
        6,
        0);

    lv_obj_set_scroll_dir(
        networkList,
        LV_DIR_VER);

    refreshPending = true;
    RefreshFromService();
}

void ScanScreen::Update()
{
    const WiFiScanState currentState =
        WiFiService::GetState();

    const uint8_t currentCount =
        WiFiService::GetNetworkCount();

    if (!refreshPending &&
        currentState == displayedState &&
        currentCount == displayedNetworkCount)
    {
        return;
    }

    RefreshFromService();
}

void ScanScreen::RefreshFromService()
{
    if (statusLabel == nullptr ||
        networkList == nullptr)
    {
        return;
    }

    const WiFiScanState state =
        WiFiService::GetState();

    const uint8_t networkCount =
        WiFiService::GetNetworkCount();

    lv_obj_clean(networkList);

    switch (state)
    {
        case WiFiScanState::Idle:
            lv_label_set_text(
                statusLabel,
                "Ready");

            AddMessageRow(
                "No scan has been completed.");
            break;

        case WiFiScanState::Scanning:
            lv_label_set_text(
                statusLabel,
                "Scanning...");

            AddMessageRow(
                "Scanning nearby Wi-Fi networks...");
            break;

        case WiFiScanState::Complete:
        {
            char statusText[24];

            std::snprintf(
                statusText,
                sizeof(statusText),
                "%u network%s",
                networkCount,
                networkCount == 1 ? "" : "s");

            lv_label_set_text(
                statusLabel,
                statusText);

            if (networkCount == 0)
            {
                AddMessageRow(
                    "No Wi-Fi networks were found.");
                break;
            }

            for (uint8_t index = 0;
                 index < networkCount;
                 ++index)
            {
                const WiFiNetworkInfo *network =
                    WiFiService::GetNetwork(index);

                if (network == nullptr)
                {
                    continue;
                }

                AddNetworkRow(*network);
            }

            break;
        }

        case WiFiScanState::Failed:
            lv_label_set_text(
                statusLabel,
                "Scan failed");

            AddMessageRow(
                "The Wi-Fi scan could not be completed.");
            break;
    }

    displayedState = state;
    displayedNetworkCount = networkCount;
    refreshPending = false;
}

void ScanScreen::AddMessageRow(
    const char *text)
{
    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        text);

    lv_label_set_long_mode(
        label,
        LV_LABEL_LONG_WRAP);

    lv_obj_set_width(
        label,
        lv_pct(100));

    lv_obj_set_style_text_color(
        label,
        Theme::Muted(),
        0);

    lv_obj_set_style_text_align(
        label,
        LV_TEXT_ALIGN_CENTER,
        0);

    lv_obj_set_style_pad_top(
        label,
        20,
        0);
}

void ScanScreen::AddNetworkRow(
    const WiFiNetworkInfo &network)
{
    const char *ssid =
        network.hidden ||
        network.ssid[0] == '\0'
            ? "<Hidden network>"
            : network.ssid;

    char rowText[128];

    std::snprintf(
        rowText,
        sizeof(rowText),
        "%s\nCH %u   %ld dBm   %s",
        ssid,
        network.channel,
        static_cast<long>(network.rssi),
        SecurityToText(network.security));

    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        rowText);

    lv_label_set_long_mode(
        label,
        LV_LABEL_LONG_WRAP);

    lv_obj_set_width(
        label,
        lv_pct(100));

    lv_obj_set_style_text_color(
        label,
        Theme::Text(),
        0);

    lv_obj_set_style_pad_bottom(
        label,
        6,
        0);

    lv_obj_set_style_border_width(
        label,
        1,
        0);

    lv_obj_set_style_border_side(
        label,
        LV_BORDER_SIDE_BOTTOM,
        0);

    lv_obj_set_style_border_color(
        label,
        Theme::Muted(),
        0);

    lv_obj_set_style_border_opa(
        label,
        LV_OPA_30,
        0);
}

const char *ScanScreen::SecurityToText(
    WiFiSecurity security)
{
    switch (security)
    {
        case WiFiSecurity::Open:
            return "Open";

        case WiFiSecurity::WEP:
            return "WEP";

        case WiFiSecurity::WPA_PSK:
            return "WPA";

        case WiFiSecurity::WPA2_PSK:
            return "WPA2";

        case WiFiSecurity::WPA_WPA2_PSK:
            return "WPA/WPA2";

        case WiFiSecurity::WPA2_Enterprise:
            return "WPA2 Enterprise";

        case WiFiSecurity::WPA3_PSK:
            return "WPA3";

        case WiFiSecurity::WPA2_WPA3_PSK:
            return "WPA2/WPA3";

        case WiFiSecurity::WAPI_PSK:
            return "WAPI";

        case WiFiSecurity::Unknown:
        default:
            return "Unknown";
    }
}

void ScanScreen::HandleMessage(
    const Message &message)
{
    if (instance == nullptr)
    {
        return;
    }

    if (message.type !=
            MessageType::WiFiScanStarted &&
        message.type !=
            MessageType::WiFiScanCompleted)
    {
        return;
    }

    // Keep synchronous MessageBus handlers short.
    // The visible screen refresh occurs during Update().
    instance->refreshPending = true;
}