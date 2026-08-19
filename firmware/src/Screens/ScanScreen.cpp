#include "ScanScreen.h"

#include "../Core/Messaging/MessageBus.h"
#include "../Services/WiFi/WiFiService.h"
#include "../Services/Storage/StorageService.h"
#include "../Services/Time/TimeService.h"
#include "../UI/Theme.h"
#include "../Services/Time/TimeService.h"

#include <Arduino.h>
#include <cstdio>

ScanScreen *ScanScreen::instance = nullptr;

namespace
{
void ConfigureControlButton(lv_obj_t *button)
{
    if (button == nullptr)
    {
        return;
    }

    lv_obj_set_style_bg_color(
        button,
        Theme::Control(),
        LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_opa(
        button,
        LV_OPA_COVER,
        LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(
        button,
        Theme::ControlPressed(),
        LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_set_style_bg_opa(
        button,
        LV_OPA_COVER,
        LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_set_style_bg_color(
        button,
        Theme::ControlDisabled(),
        LV_PART_MAIN | LV_STATE_DISABLED);

    lv_obj_set_style_bg_opa(
        button,
        LV_OPA_COVER,
        LV_PART_MAIN | LV_STATE_DISABLED);

    lv_obj_set_style_opa(
        button,
        LV_OPA_60,
        LV_PART_MAIN | LV_STATE_DISABLED);

    lv_obj_set_style_border_width(
        button,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_border_color(
        button,
        Theme::ControlSelected(),
        LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ConfigureControlLabel(lv_obj_t *label)
{
    if (label == nullptr)
    {
        return;
    }

    lv_obj_set_style_text_color(
        label,
        Theme::ControlText(),
        0);
}

void SetControlSelected(lv_obj_t *button, bool selected)
{
    if (button == nullptr)
    {
        return;
    }

    lv_obj_set_style_border_width(
        button,
        selected ? 2 : 0,
        LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_border_opa(
        button,
        selected ? LV_OPA_COVER : LV_OPA_TRANSP,
        LV_PART_MAIN | LV_STATE_DEFAULT);
}
}

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

    if (!MessageBus::Subscribe(
            MessageType::WiFiMeasurementSessionCompleted,
            ScanScreen::HandleMessage))
    {
        Serial.println(
            "ScanScreen: Failed to subscribe "
            "to WiFiMeasurementSessionCompleted");
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
        4,
        0);

    lv_obj_set_style_pad_row(
        layout,
        4,
        0);

    lv_obj_set_flex_flow(
        layout,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        layout,
        LV_OBJ_FLAG_SCROLLABLE);

    // Compact single-row toolbar. The ApplicationFrame already
    // identifies the current page, so the repeated "Wi-Fi Scan"
    // title is omitted to preserve vertical result space.
    lv_obj_t *header =
        lv_obj_create(layout);

    lv_obj_remove_style_all(header);

    lv_obj_set_width(
        header,
        lv_pct(100));

    lv_obj_set_height(
        header,
        30);

    lv_obj_set_flex_flow(
        header,
        LV_FLEX_FLOW_ROW);

    lv_obj_set_flex_align(
        header,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_column(
        header,
        6,
        0);

    lv_obj_clear_flag(
        header,
        LV_OBJ_FLAG_SCROLLABLE);

    networksButton =
        lv_btn_create(header);

    ConfigureControlButton(networksButton);

    lv_obj_set_size(
        networksButton,
        88,
        28);

    lv_obj_add_event_cb(
        networksButton,
        ScanScreen::HandleNetworksButton,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_t *networksLabel =
        lv_label_create(networksButton);

    lv_label_set_text(
        networksLabel,
        "Networks");

    ConfigureControlLabel(networksLabel);

    lv_obj_center(networksLabel);

    channelsButton =
        lv_btn_create(header);

    ConfigureControlButton(channelsButton);

    lv_obj_set_size(
        channelsButton,
        88,
        28);

    lv_obj_add_event_cb(
        channelsButton,
        ScanScreen::HandleChannelsButton,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_t *channelsLabel =
        lv_label_create(channelsButton);

    lv_label_set_text(
        channelsLabel,
        "Channels");

    ConfigureControlLabel(channelsLabel);

    lv_obj_center(channelsLabel);

    statusLabel =
        lv_label_create(header);

    lv_label_set_text(
        statusLabel,
        "Loading");

    lv_obj_set_flex_grow(
        statusLabel,
        1);

    lv_obj_set_style_text_color(
        statusLabel,
        Theme::Muted(),
        0);

    lv_obj_set_style_text_align(
        statusLabel,
        LV_TEXT_ALIGN_RIGHT,
        0);

    // Saved-session review is entered from the existing toolbar.
    // It does not consume an additional row of result space.
    historyButton =
        lv_btn_create(header);

    ConfigureControlButton(historyButton);

    lv_obj_set_size(
        historyButton,
        64,
        28);

    lv_obj_add_event_cb(
        historyButton,
        ScanScreen::HandleHistoryButton,
        LV_EVENT_CLICKED,
        nullptr);

    historyButtonLabel =
        lv_label_create(historyButton);

    lv_label_set_text(
        historyButtonLabel,
        "History");

    ConfigureControlLabel(historyButtonLabel);

    lv_obj_center(historyButtonLabel);

    lv_obj_add_flag(
        historyButton,
        LV_OBJ_FLAG_HIDDEN);

    newSessionButton =
        lv_btn_create(header);

    ConfigureControlButton(newSessionButton);

    lv_obj_set_size(
        newSessionButton,
        62,
        28);

    lv_obj_add_event_cb(
        newSessionButton,
        ScanScreen::HandleNewSessionButton,
        LV_EVENT_CLICKED,
        nullptr);

    newSessionButtonLabel =
        lv_label_create(newSessionButton);

    lv_label_set_text(
        newSessionButtonLabel,
        "New");

    ConfigureControlLabel(newSessionButtonLabel);

    lv_obj_center(newSessionButtonLabel);

    scanButton =
        lv_btn_create(header);

    ConfigureControlButton(scanButton);

    lv_obj_set_size(
        scanButton,
        72,
        28);

    lv_obj_add_event_cb(
        scanButton,
        ScanScreen::HandleScanButton,
        LV_EVENT_CLICKED,
        nullptr);

    scanButtonLabel =
        lv_label_create(scanButton);

    lv_label_set_text(
        scanButtonLabel,
        "Scan");

    ConfigureControlLabel(scanButtonLabel);

    lv_obj_center(scanButtonLabel);

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
        2,
        0);

    lv_obj_set_style_pad_row(
        networkList,
        4,
        0);

    lv_obj_set_scroll_dir(
        networkList,
        LV_DIR_VER);

    refreshPending = true;

    UpdateViewButtons();
    UpdateHistoryControls();
    RefreshFromService();
}

void ScanScreen::Update()
{
    const WiFiScanState currentState =
        WiFiService::GetState();

    const WiFiMeasurementSessionState
        currentSessionState =
            WiFiService::
                GetMeasurementSessionState();

    const uint8_t currentCount =
        WiFiService::GetNetworkCount();

    const uint8_t currentSavedSessionCount =
        StorageService::GetSavedSessionCount();

    if (!refreshPending &&
        currentState == displayedState &&
        currentSessionState ==
            displayedSessionState &&
        currentCount == displayedNetworkCount &&
        currentSavedSessionCount ==
            displayedSavedSessionCount)
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

    const WiFiMeasurementSessionState sessionState =
        WiFiService::GetMeasurementSessionState();

    const uint8_t completedSessionScans =
        WiFiService::
            GetMeasurementSessionCompletedScanCount();

    const uint8_t networkCount =
        WiFiService::GetNetworkCount();

    if (sessionState ==
            WiFiMeasurementSessionState::Complete &&
        displayedSessionState !=
            WiFiMeasurementSessionState::Complete)
    {
        // Surface the completed field summary once when the
        // automatic measurement session finishes.
        activeView =
            WiFiScanView::Channels;
    }

    UpdateScanButton(
        state,
        sessionState);

    UpdateNewSessionButton(
        state,
        sessionState);

    UpdateViewButtons();
    UpdateHistoryControls();

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
        {
            if (sessionState ==
                WiFiMeasurementSessionState::Cancelling)
            {
                lv_label_set_text(
                    statusLabel,
                    "Stopping...");

                AddMessageRow(
                    "Finishing the active scan before "
                    "stopping the automatic session...");
            }
            else if (sessionState ==
                     WiFiMeasurementSessionState::Running)
            {
                const uint8_t currentScan =
                    completedSessionScans + 1 <=
                            WiFiService::
                                AutomaticSessionScanCount
                        ? completedSessionScans + 1
                        : WiFiService::
                              AutomaticSessionScanCount;

                char statusText[24];
                char messageText[96];

                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "Scan %u/%u...",
                    currentScan,
                    WiFiService::
                        AutomaticSessionScanCount);

                std::snprintf(
                    messageText,
                    sizeof(messageText),
                    "Automatic measurement scan %u of %u...",
                    currentScan,
                    WiFiService::
                        AutomaticSessionScanCount);

                lv_label_set_text(
                    statusLabel,
                    statusText);

                AddMessageRow(
                    messageText);
            }
            else
            {
                lv_label_set_text(
                    statusLabel,
                    "Scanning...");

                AddMessageRow(
                    "Scanning nearby Wi-Fi networks...");
            }

            break;
        }

        case WiFiScanState::Complete:
        {
            char statusText[32];

            if (reviewingSavedSession)
            {
                const uint8_t savedCount =
                    StorageService::GetSavedSessionCount();

                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "Saved %u/%u",
                    selectedSavedSessionIndex + 1,
                    savedCount);
            }
            else if (sessionState ==
                WiFiMeasurementSessionState::Running)
            {
                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "%u/%u complete",
                    completedSessionScans,
                    WiFiService::
                        AutomaticSessionScanCount);
            }
            else if (sessionState ==
                     WiFiMeasurementSessionState::Complete)
            {
                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "%u/%u complete",
                    WiFiService::
                        AutomaticSessionScanCount,
                    WiFiService::
                        AutomaticSessionScanCount);
            }
            else if (sessionState ==
                     WiFiMeasurementSessionState::Cancelled)
            {
                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "%u/%u stopped",
                    completedSessionScans,
                    WiFiService::
                        AutomaticSessionScanCount);
            }
            else if (activeView ==
                     WiFiScanView::Networks)
            {
                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "%u network%s",
                    networkCount,
                    networkCount == 1 ? "" : "s");
            }
            else
            {
                const uint8_t occupiedChannels =
                    WiFiService::GetOccupiedChannelCount();

                std::snprintf(
                    statusText,
                    sizeof(statusText),
                    "%u channel%s",
                    occupiedChannels,
                    occupiedChannels == 1 ? "" : "s");
            }

            lv_label_set_text(
                statusLabel,
                statusText);

            if (activeView ==
                WiFiScanView::Networks)
            {
                ShowNetworkResults(
                    networkCount);
            }
            else
            {
                ShowChannelResults();
            }

            break;
        }

        case WiFiScanState::Failed:
            lv_label_set_text(
                statusLabel,
                sessionState ==
                        WiFiMeasurementSessionState::Failed
                    ? "Session failed"
                    : "Scan failed");

            AddMessageRow(
                sessionState ==
                        WiFiMeasurementSessionState::Failed
                    ? "The automatic measurement session "
                      "could not be completed."
                    : "The Wi-Fi scan could not be completed.");
            break;
    }

    displayedState = state;
    displayedSessionState = sessionState;
    displayedNetworkCount = networkCount;
    displayedSavedSessionCount =
        StorageService::GetSavedSessionCount();
    refreshPending = false;
}

void ScanScreen::UpdateScanButton(
    WiFiScanState state,
    WiFiMeasurementSessionState sessionState)
{
    if (scanButton == nullptr ||
        scanButtonLabel == nullptr)
    {
        return;
    }

    const bool automaticSessionActive =
        sessionState ==
            WiFiMeasurementSessionState::Running ||
        sessionState ==
            WiFiMeasurementSessionState::Cancelling;

    if (state == WiFiScanState::Scanning ||
        automaticSessionActive)
    {
        lv_obj_add_state(
            scanButton,
            LV_STATE_DISABLED);

        lv_label_set_text(
            scanButtonLabel,
            state == WiFiScanState::Scanning
                ? "Scanning"
                : "Auto");
    }
    else
    {
        lv_obj_clear_state(
            scanButton,
            LV_STATE_DISABLED);

        lv_label_set_text(
            scanButtonLabel,
            "Scan");
    }
}

void ScanScreen::UpdateNewSessionButton(
    WiFiScanState state,
    WiFiMeasurementSessionState sessionState)
{
    if (newSessionButton == nullptr ||
        newSessionButtonLabel == nullptr)
    {
        return;
    }

    if (sessionState ==
        WiFiMeasurementSessionState::Running)
    {
        lv_obj_clear_state(
            newSessionButton,
            LV_STATE_DISABLED);

        lv_label_set_text(
            newSessionButtonLabel,
            "Cancel");

        return;
    }

    if (sessionState ==
        WiFiMeasurementSessionState::Cancelling)
    {
        lv_obj_add_state(
            newSessionButton,
            LV_STATE_DISABLED);

        lv_label_set_text(
            newSessionButtonLabel,
            "Wait");

        return;
    }

    lv_label_set_text(
        newSessionButtonLabel,
        "New");

    if (state == WiFiScanState::Scanning)
    {
        lv_obj_add_state(
            newSessionButton,
            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_clear_state(
            newSessionButton,
            LV_STATE_DISABLED);
    }
}

void ScanScreen::UpdateViewButtons()
{
    if (networksButton == nullptr ||
        channelsButton == nullptr)
    {
        return;
    }

    const bool showingNetworks =
        activeView ==
        WiFiScanView::Networks;

    // Both view controls remain visibly enabled. The active view is
    // identified by a light-teal border rather than by changing the
    // inactive control to a disabled-looking grey.
    SetControlSelected(
        networksButton,
        showingNetworks);

    SetControlSelected(
        channelsButton,
        !showingNetworks);
}

void ScanScreen::UpdateHistoryControls()
{
    if (historyButton == nullptr ||
        historyButtonLabel == nullptr ||
        newSessionButton == nullptr ||
        newSessionButtonLabel == nullptr ||
        scanButton == nullptr ||
        scanButtonLabel == nullptr)
    {
        return;
    }

    const uint8_t savedCount =
        StorageService::GetSavedSessionCount();

    const bool measurementActive =
        WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive();

    if (savedCount == 0)
    {
        reviewingSavedSession = false;
        selectedSavedSessionIndex = 0;

        SetControlSelected(
            historyButton,
            false);

        lv_obj_add_flag(
            historyButton,
            LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Saved history is a persistent capability and should be visible
    // immediately after it is restored from the SD card. It is no
    // longer tied to the Channels view or to completion of a new scan.
    lv_obj_clear_flag(
        historyButton,
        LV_OBJ_FLAG_HIDDEN);

    if (selectedSavedSessionIndex >= savedCount)
    {
        selectedSavedSessionIndex =
            savedCount - 1;
    }

    if (!reviewingSavedSession)
    {
        lv_label_set_text(
            historyButtonLabel,
            "History");

        SetControlSelected(
            historyButton,
            false);

        if (measurementActive)
        {
            lv_obj_add_state(
                historyButton,
                LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_clear_state(
                historyButton,
                LV_STATE_DISABLED);
        }

        return;
    }


    lv_obj_clear_state(
        historyButton,
        LV_STATE_DISABLED);

    // In history mode, the normal New and Scan buttons become
    // Older and Newer. This preserves all vertical result space.
    lv_label_set_text(
        historyButtonLabel,
        "Live");

    SetControlSelected(
        historyButton,
        true);

    lv_label_set_text(
        newSessionButtonLabel,
        "Older");

    lv_label_set_text(
        scanButtonLabel,
        "Newer");

    if (selectedSavedSessionIndex + 1 < savedCount)
    {
        lv_obj_clear_state(
            newSessionButton,
            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(
            newSessionButton,
            LV_STATE_DISABLED);
    }

    if (selectedSavedSessionIndex > 0)
    {
        lv_obj_clear_state(
            scanButton,
            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(
            scanButton,
            LV_STATE_DISABLED);
    }
}

void ScanScreen::ShowNetworkResults(
    uint8_t networkCount)
{
    // When reviewing History, display the network inventory stored
    // with the selected measurement session rather than the current
    // live Wi-Fi scan cache.
    if (reviewingSavedSession)
    {
        const StoredWiFiMeasurementSession *saved =
            StorageService::GetSavedSession(
                selectedSavedSessionIndex);

        if (saved != nullptr &&
            saved->available &&
            saved->summary.available)
        {
            AddSavedSessionHeader(
                saved->sessionId,
                saved->capturedTimeValid,
                saved->capturedEpoch,
                saved->completedAtMs,
                saved->summary.siteSurveyName,
                saved->summary.surveyPoint);

            const WiFiMeasurementSummary &summary =
                saved->summary;

            const uint8_t observedCount =
                summary.observedNetworkCount <=
                        WiFiMeasurementSummary::NetworkCapacity
                    ? summary.observedNetworkCount
                    : WiFiMeasurementSummary::NetworkCapacity;

            if (observedCount == 0)
            {
                AddMessageRow(
                    "No Wi-Fi networks were recorded "
                    "for this session.");

                return;
            }

            AddSectionLabel(
                "OBSERVED NETWORKS");

            for (uint8_t index = 0;
                 index < observedCount;
                 ++index)
            {
                const WiFiMeasuredNetwork &measured =
                    summary.networks[index];

                // Reuse the existing live-network presentation.
                // For a stored measurement session, RSSI represents
                // the average signal observed across the session.
                WiFiNetworkInfo displayNetwork{};

                std::snprintf(
                    displayNetwork.ssid,
                    sizeof(displayNetwork.ssid),
                    "%s",
                    measured.ssid);

                for (uint8_t byte = 0;
                     byte < WiFiNetworkInfo::BssidLength;
                     ++byte)
                {
                    displayNetwork.bssid[byte] =
                        measured.bssid[byte];
                }

                displayNetwork.rssi =
                    measured.averageRssi;

                displayNetwork.channel =
                    measured.channel;

                displayNetwork.signalQuality =
                    measured.signalQuality;

                displayNetwork.security =
                    measured.security;

                displayNetwork.hidden =
                    measured.hidden;

                char bssidText[18];

                std::snprintf(
                    bssidText,
                    sizeof(bssidText),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    static_cast<unsigned>(measured.bssid[0]),
                    static_cast<unsigned>(measured.bssid[1]),
                    static_cast<unsigned>(measured.bssid[2]),
                    static_cast<unsigned>(measured.bssid[3]),
                    static_cast<unsigned>(measured.bssid[4]),
                    static_cast<unsigned>(measured.bssid[5]));

                char detailText[128];

                std::snprintf(
                    detailText,
                    sizeof(detailText),
                    "Seen %u/%u scans   Range %ld to %ld dBm\n"
                    "BSSID %s",
                    static_cast<unsigned>(measured.seenCount),
                    static_cast<unsigned>(summary.completedScanCount),
                    static_cast<long>(measured.minimumRssi),
                    static_cast<long>(measured.maximumRssi),
                    bssidText);

                AddNetworkRow(
                    displayNetwork,
                    detailText,
                    true);
            }

            return;
        }

        // If the stored session cannot be loaded, return to the
        // normal live presentation.
        reviewingSavedSession = false;
        UpdateHistoryControls();
    }

    // Normal live-scan presentation.
    if (networkCount == 0)
    {
        AddMessageRow(
            "No Wi-Fi networks were found.");

        return;
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
}

void ScanScreen::ShowChannelResults()
{
    if (reviewingSavedSession)
    {
        const StoredWiFiMeasurementSession *saved =
            StorageService::GetSavedSession(
                selectedSavedSessionIndex);

        if (saved != nullptr &&
            saved->available &&
            saved->summary.available)
        {
            AddSavedSessionHeader(
                saved->sessionId,
                saved->capturedTimeValid,
                saved->capturedEpoch,
                saved->completedAtMs,
                saved->summary.siteSurveyName,
                saved->summary.surveyPoint);

            ShowMeasurementSummary(
                saved->summary);
            return;
        }

        reviewingSavedSession = false;
        UpdateHistoryControls();
    }

    const WiFiMeasurementSessionState sessionState =
        WiFiService::GetMeasurementSessionState();

    const WiFiMeasurementSummary &summary =
        WiFiService::GetMeasurementSummary();

    if (sessionState ==
            WiFiMeasurementSessionState::Complete &&
        summary.available)
    {
        ShowMeasurementSummary(summary);
        return;
    }

    const WiFiChannelRecommendation
        &recommendation =
            WiFiService::GetChannelRecommendation();

    AddRecommendationRow(recommendation);

    for (uint8_t index = 0;
         index < WiFiService::CandidateChannelCount;
         ++index)
    {
        const WiFiChannelAssessment *assessment =
            WiFiService::GetCandidateAssessment(index);

        if (assessment == nullptr)
        {
            continue;
        }

        AddCandidateAssessmentRow(*assessment);
    }

    const uint8_t occupiedChannels =
        WiFiService::GetOccupiedChannelCount();

    if (occupiedChannels == 0)
    {
        AddMessageRow(
            "No occupied Wi-Fi channels were found.");
        return;
    }

    for (uint8_t channel = 1;
         channel <= WiFiService::Max2_4GHzChannel;
         ++channel)
    {
        const WiFiChannelInfo *info =
            WiFiService::GetChannelInfo(channel);

        if (info == nullptr ||
            info->networkCount == 0)
        {
            continue;
        }

        AddChannelRow(*info);
    }
}

void ScanScreen::ShowMeasurementSummary(
    const WiFiMeasurementSummary &summary)
{
    AddCompletedSessionSummary(summary);

    for (uint8_t index = 0;
         index <
             WiFiMeasurementSummary::CandidateCapacity;
         ++index)
    {
        AddCandidateAssessmentRow(
            summary.candidates[index]);
    }

    AddSectionLabel(
        "FINAL SCAN CHANNEL DETAIL");

    for (uint8_t channel = 1;
         channel <
             WiFiMeasurementSummary::ChannelCapacity;
         ++channel)
    {
        const WiFiChannelInfo &info =
            summary.channels[channel];

        if (info.networkCount == 0)
        {
            continue;
        }

        AddChannelRow(info);
    }
}

void ScanScreen::AddSavedSessionHeader(
    uint32_t sessionId,
    bool capturedTimeValid,
    uint32_t capturedEpoch,
    uint32_t completedAtMs,
    const char *siteSurveyName,
    const char *surveyPoint)
{
    char text[128];

    // ------------------------------------------------------------
    // Saved session date/time
    // ------------------------------------------------------------

    bool headerAdded = false;

    if (capturedTimeValid &&
        capturedEpoch != 0)
    {
        char capturedText[40] = {};

        if (TimeService::FormatEpochForHistory(
                capturedEpoch,
                capturedText,
                sizeof(capturedText)))
        {
            std::snprintf(
                text,
                sizeof(text),
                "SAVED SESSION #%lu   %s",
                static_cast<unsigned long>(
                    sessionId),
                capturedText);

            AddSectionLabel(text);
            headerAdded = true;
        }
    }

    // Legacy sessions do not have a wall-clock timestamp.
    if (!headerAdded)
    {
        const uint32_t totalSeconds =
            completedAtMs / 1000UL;

        const uint32_t hours =
            totalSeconds / 3600UL;

        const uint32_t minutes =
            (totalSeconds / 60UL) % 60UL;

        const uint32_t seconds =
            totalSeconds % 60UL;

        std::snprintf(
            text,
            sizeof(text),
            "SAVED SESSION #%lu   "
            "Legacy uptime %02lu:%02lu:%02lu",
            static_cast<unsigned long>(
                sessionId),
            static_cast<unsigned long>(
                hours),
            static_cast<unsigned long>(
                minutes),
            static_cast<unsigned long>(
                seconds));

        AddSectionLabel(text);
    }

    // ------------------------------------------------------------
    // Site Survey
    // ------------------------------------------------------------

    if (siteSurveyName != nullptr &&
        siteSurveyName[0] != '\0')
    {
        std::snprintf(
            text,
            sizeof(text),
            "SITE SURVEY: %s",
            siteSurveyName);

        AddSectionLabel(text);
    }

    // ------------------------------------------------------------
    // Survey Point
    // ------------------------------------------------------------

    if (surveyPoint != nullptr &&
        surveyPoint[0] != '\0')
    {
        std::snprintf(
            text,
            sizeof(text),
            "SURVEY POINT: %s",
            surveyPoint);

        AddSectionLabel(text);
    }
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
    const WiFiNetworkInfo &network,
    const char *detailText,
    bool historicalAverage)

{
    const char *ssid =
        network.hidden ||
        network.ssid[0] == '\0'
            ? "<Hidden network>"
            : network.ssid;

    char rowText[224];

    const char *rssiPrefix =
        historicalAverage
            ? "Avg "
            : "";

    if (detailText != nullptr &&
        detailText[0] != '\0')
    {
        std::snprintf(
            rowText,
            sizeof(rowText),
            "%s\n%s   CH %u   %s%ld dBm   %s\n%s",
            ssid,
            SignalQualityToText(
                network.signalQuality),
            network.channel,
            rssiPrefix,
            static_cast<long>(network.rssi),
            SecurityToText(network.security),
            detailText);
    }
    else
    {
        std::snprintf(
            rowText,
            sizeof(rowText),
            "%s\n%s   CH %u   %s%ld dBm   %s",
            ssid,
            SignalQualityToText(
                network.signalQuality),
            network.channel,
            rssiPrefix,
            static_cast<long>(network.rssi),
            SecurityToText(network.security));
    }

    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        rowText);

    lv_label_set_recolor(
        label,
        true);

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


void ScanScreen::AddRecommendationRow(
    const WiFiChannelRecommendation
        &recommendation)
{
    char channelText[48] = {};
    size_t used = 0;

    for (uint8_t index = 0;
         index <
             WiFiService::CandidateChannelCount;
         ++index)
    {
        const WiFiChannelAssessment *assessment =
            WiFiService::GetCandidateAssessment(
                index);

        if (assessment == nullptr ||
            !assessment->comparable)
        {
            continue;
        }

        const int written =
            std::snprintf(
                channelText + used,
                sizeof(channelText) - used,
                used == 0
                    ? "CH %u"
                    : " / CH %u",
                assessment->channel);

        if (written <= 0)
        {
            continue;
        }

        const size_t available =
            sizeof(channelText) - used;

        const size_t appended =
            static_cast<size_t>(written) >=
                    available
                ? available - 1
                : static_cast<size_t>(written);

        used += appended;

        if (used >=
            sizeof(channelText) - 1)
        {
            break;
        }
    }

    char rowText[192];

    if (recommendation.unique)
    {
        std::snprintf(
            rowText,
            sizeof(rowText),
            "Recommended: CH %u   Avg %u\n"
            "Margin %u   Confidence %s   %u/%u scans",
            recommendation.bestChannel,
            recommendation.bestScore,
            recommendation.scoreMargin,
            RecommendationConfidenceToText(
                recommendation.confidence),
            recommendation.historySampleCount,
            WiFiService::ScoreHistoryDepth);
    }
    else
    {
        std::snprintf(
            rowText,
            sizeof(rowText),
            "Comparable: %s\n"
            "Best CH %u   Avg %u   Confidence %s   %u/%u",
            channelText[0] == '\0'
                ? "None"
                : channelText,
            recommendation.bestChannel,
            recommendation.bestScore,
            RecommendationConfidenceToText(
                recommendation.confidence),
            recommendation.historySampleCount,
            WiFiService::ScoreHistoryDepth);
    }

    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        rowText);

    lv_label_set_recolor(
        label,
        true);

    lv_label_set_long_mode(
        label,
        LV_LABEL_LONG_WRAP);

    lv_obj_set_width(
        label,
        lv_pct(100));

    lv_obj_set_style_text_color(
        label,
        Theme::Accent(),
        0);

    lv_obj_set_style_pad_all(
        label,
        6,
        0);

    lv_obj_set_style_border_width(
        label,
        1,
        0);

    lv_obj_set_style_border_color(
        label,
        Theme::Accent(),
        0);

    lv_obj_set_style_border_opa(
        label,
        LV_OPA_70,
        0);

    lv_obj_set_style_radius(
        label,
        4,
        0);
}

void ScanScreen::AddCompletedSessionSummary(
    const WiFiMeasurementSummary &summary)
{
    const WiFiChannelRecommendation
        &recommendation =
            summary.recommendation;

    char alternatives[48] = {};
    size_t used = 0;

    for (uint8_t index = 0;
         index <
             WiFiMeasurementSummary::
                 CandidateCapacity;
         ++index)
    {
        const WiFiChannelAssessment &assessment =
            summary.candidates[index];

        if (!assessment.comparable ||
            assessment.recommended)
        {
            continue;
        }

        const int written =
            std::snprintf(
                alternatives + used,
                sizeof(alternatives) - used,
                used == 0
                    ? "CH %u"
                    : " / CH %u",
                assessment.channel);

        if (written <= 0)
        {
            continue;
        }

        const size_t available =
            sizeof(alternatives) - used;

        const size_t appended =
            static_cast<size_t>(written) >=
                    available
                ? available - 1
                : static_cast<size_t>(written);

        used += appended;

        if (used >=
            sizeof(alternatives) - 1)
        {
            break;
        }
    }

    char rowText[256];

    if (recommendation.unique)
    {
        std::snprintf(
            rowText,
            sizeof(rowText),
            "#00E676 SESSION COMPLETE#   %u/%u scans\n"
            "Recommended CH %u   Confidence %s\n"
            "Avg %u   Margin %u\n"
            "Final scan: %u networks / %u channels   "
            "Alt %s",
            summary.completedScanCount,
            WiFiService::AutomaticSessionScanCount,
            recommendation.bestChannel,
            RecommendationConfidenceToText(
                recommendation.confidence),
            recommendation.bestScore,
            recommendation.scoreMargin,
            summary.networkCount,
            summary.occupiedChannelCount,
            alternatives[0] == '\0'
                ? "None"
                : alternatives);
    }
    else
    {
        char comparable[48] = {};
        size_t comparableUsed = 0;

        for (uint8_t index = 0;
             index <
                 WiFiMeasurementSummary::
                     CandidateCapacity;
             ++index)
        {
            const WiFiChannelAssessment &assessment =
                summary.candidates[index];

            if (!assessment.comparable)
            {
                continue;
            }

            const int written =
                std::snprintf(
                    comparable + comparableUsed,
                    sizeof(comparable) -
                        comparableUsed,
                    comparableUsed == 0
                        ? "CH %u"
                        : " / CH %u",
                    assessment.channel);

            if (written <= 0)
            {
                continue;
            }

            const size_t available =
                sizeof(comparable) -
                comparableUsed;

            const size_t appended =
                static_cast<size_t>(written) >=
                        available
                    ? available - 1
                    : static_cast<size_t>(written);

            comparableUsed += appended;

            if (comparableUsed >=
                sizeof(comparable) - 1)
            {
                break;
            }
        }

        std::snprintf(
            rowText,
            sizeof(rowText),
            "#00E676 SESSION COMPLETE#   %u/%u scans\n"
            "Comparable %s   Confidence %s\n"
            "Best observed CH %u   Avg %u   Margin %u\n"
            "Final scan: %u networks / %u channels",
            summary.completedScanCount,
            WiFiService::AutomaticSessionScanCount,
            comparable[0] == '\0'
                ? "None"
                : comparable,
            RecommendationConfidenceToText(
                recommendation.confidence),
            recommendation.bestChannel,
            recommendation.bestScore,
            recommendation.scoreMargin,
            summary.networkCount,
            summary.occupiedChannelCount);
    }

    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        rowText);

    lv_label_set_recolor(
        label,
        true);

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

    lv_obj_set_style_pad_all(
        label,
        8,
        0);

    lv_obj_set_style_border_width(
        label,
        1,
        0);

    lv_obj_set_style_border_color(
        label,
        Theme::Accent(),
        0);

    lv_obj_set_style_border_opa(
        label,
        LV_OPA_COVER,
        0);

    lv_obj_set_style_radius(
        label,
        4,
        0);
}

void ScanScreen::AddSectionLabel(
    const char *text)
{
    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        text);

    lv_obj_set_width(
        label,
        lv_pct(100));

    lv_obj_set_style_text_color(
        label,
        Theme::Muted(),
        0);

    lv_obj_set_style_pad_top(
        label,
        4,
        0);

    lv_obj_set_style_pad_bottom(
        label,
        2,
        0);
}

void ScanScreen::AddCandidateAssessmentRow(
    const WiFiChannelAssessment &assessment)
{
    char rowText[144];

    const char *marker =
        assessment.recommended
            ? "BEST"
            : assessment.comparable
                ? "ALT"
                : "";

    std::snprintf(
        rowText,
        sizeof(rowText),
        "CH %u   Avg %u   Now %u   %s%s%s",
        assessment.channel,
        assessment.congestionScore,
        assessment.latestScore,
        CongestionLevelToText(
            assessment.congestion),
        marker[0] == '\0' ? "" : "   ",
        marker);

    lv_obj_t *label =
        lv_label_create(networkList);

    lv_label_set_text(
        label,
        rowText);

    lv_label_set_recolor(
        label,
        true);

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
        4,
        0);
}

void ScanScreen::AddChannelRow(
    const WiFiChannelInfo &channelInfo)
{
    char rowText[128];

    std::snprintf(
        rowText,
        sizeof(rowText),
        "Channel %u\n"
        "%u network%s   Strongest %ld dBm   "
        "Average %ld dBm",
        channelInfo.channel,
        channelInfo.networkCount,
        channelInfo.networkCount == 1
            ? ""
            : "s",
        static_cast<long>(
            channelInfo.strongestRssi),
        static_cast<long>(
            channelInfo.averageRssi));

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
        8,
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

const char *ScanScreen::SignalQualityToText(
    WiFiSignalQuality quality)
{
    switch (quality)
    {
        case WiFiSignalQuality::Excellent:
            return "#00E676 Excellent#";

        case WiFiSignalQuality::Good:
            return "#FFD740 Good#";

        case WiFiSignalQuality::Fair:
            return "#FF9800 Fair#";

        case WiFiSignalQuality::Poor:
            return "#FF5252 Poor#";

        case WiFiSignalQuality::Unknown:
        default:
            return "#9E9E9E Unknown#";
    }
}

const char *ScanScreen::CongestionLevelToText(
    WiFiCongestionLevel congestion)
{
    switch (congestion)
    {
        case WiFiCongestionLevel::Excellent:
            return "#00E676 Excellent#";

        case WiFiCongestionLevel::Good:
            return "#FFD740 Good#";

        case WiFiCongestionLevel::Fair:
            return "#FF9800 Fair#";

        case WiFiCongestionLevel::Poor:
            return "#FF5252 Poor#";

        case WiFiCongestionLevel::Unknown:
        default:
            return "#9E9E9E Unknown#";
    }
}

const char *ScanScreen::RecommendationConfidenceToText(
    WiFiRecommendationConfidence confidence)
{
    switch (confidence)
    {
        case WiFiRecommendationConfidence::High:
            return "#00E676 High#";
        case WiFiRecommendationConfidence::Medium:
            return "#FFD740 Medium#";
        case WiFiRecommendationConfidence::Low:
            return "#FF9800 Low#";
        case WiFiRecommendationConfidence::Unknown:
        default:
            return "#9E9E9E Unknown#";
    }
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

void ScanScreen::HandleScanButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr)
    {
        return;
    }

    if (lv_event_get_code(event) !=
        LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->reviewingSavedSession)
    {
        if (instance->selectedSavedSessionIndex > 0)
        {
            --instance->selectedSavedSessionIndex;
            instance->refreshPending = true;
        }

        return;
    }

    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return;
    }

    instance->measurementSetupScreen.Show(
        MeasurementSetupAction::SingleScan);

    instance->refreshPending = true;
}

void ScanScreen::HandleNewSessionButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->reviewingSavedSession)
    {
        const uint8_t savedCount =
            StorageService::GetSavedSessionCount();

        if (instance->selectedSavedSessionIndex + 1 <
            savedCount)
        {
            ++instance->selectedSavedSessionIndex;
            instance->refreshPending = true;
        }

        return;
    }

    const WiFiMeasurementSessionState sessionState =
        WiFiService::GetMeasurementSessionState();

    if (sessionState ==
        WiFiMeasurementSessionState::Running)
    {
        if (!WiFiService::
                CancelAutomaticMeasurementSession())
        {
            Serial.println(
                "ScanScreen: Automatic session "
                "cancel request failed");
        }

        instance->refreshPending = true;
        return;
    }

    if (sessionState ==
        WiFiMeasurementSessionState::Cancelling)
    {
        return;
    }

    instance->reviewingSavedSession = false;
    instance->selectedSavedSessionIndex = 0;

    instance->measurementSetupScreen.Show(
        MeasurementSetupAction::MeasurementSession);

    instance->refreshPending = true;
}

void ScanScreen::HandleNetworksButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->activeView ==
        WiFiScanView::Networks)
    {
        return;
    }

    instance->activeView =
        WiFiScanView::Networks;

    instance->UpdateViewButtons();
    instance->refreshPending = true;
}

void ScanScreen::HandleChannelsButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->activeView ==
        WiFiScanView::Channels)
    {
        return;
    }

    instance->activeView =
        WiFiScanView::Channels;

    instance->UpdateViewButtons();
    instance->refreshPending = true;
}

void ScanScreen::HandleHistoryButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return;
    }

    const uint8_t savedCount =
        StorageService::GetSavedSessionCount();

    if (savedCount == 0)
    {
        return;
    }

    if (instance->reviewingSavedSession)
    {
        instance->reviewingSavedSession = false;
        instance->selectedSavedSessionIndex = 0;
    }
    else
    {
        instance->reviewingSavedSession = true;
        instance->selectedSavedSessionIndex = 0;
    }

    instance->UpdateViewButtons();
    instance->refreshPending = true;
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
            MessageType::WiFiScanCompleted &&
        message.type !=
            MessageType::WiFiMeasurementSessionCompleted)
    {
        return;
    }

    // Keep synchronous MessageBus handlers short.
    // The visible screen refresh occurs during Update().
    instance->refreshPending = true;
}
