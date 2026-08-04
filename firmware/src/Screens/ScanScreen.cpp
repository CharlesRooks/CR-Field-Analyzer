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

    lv_obj_set_style_text_color(
        networksLabel,
        Theme::Text(),
        0);

    lv_obj_center(networksLabel);

    channelsButton =
        lv_btn_create(header);

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

    lv_obj_set_style_text_color(
        channelsLabel,
        Theme::Text(),
        0);

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

    newSessionButton =
        lv_btn_create(header);

    lv_obj_set_size(
        newSessionButton,
        62,
        28);

    lv_obj_set_style_bg_color(
        newSessionButton,
        Theme::Muted(),
        0);

    lv_obj_set_style_bg_opa(
        newSessionButton,
        LV_OPA_50,
        0);

    lv_obj_set_style_opa(
        newSessionButton,
        LV_OPA_30,
        LV_PART_MAIN | LV_STATE_DISABLED);

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

    lv_obj_set_style_text_color(
        newSessionButtonLabel,
        Theme::Text(),
        0);

    lv_obj_center(newSessionButtonLabel);

    scanButton =
        lv_btn_create(header);

    lv_obj_set_size(
        scanButton,
        72,
        28);

    lv_obj_set_style_bg_color(
        scanButton,
        Theme::Accent(),
        0);

    lv_obj_set_style_bg_opa(
        scanButton,
        LV_OPA_COVER,
        0);

    lv_obj_set_style_opa(
        scanButton,
        LV_OPA_50,
        LV_PART_MAIN | LV_STATE_DISABLED);

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

    lv_obj_set_style_text_color(
        scanButtonLabel,
        Theme::Text(),
        0);

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

    if (!refreshPending &&
        currentState == displayedState &&
        currentSessionState ==
            displayedSessionState &&
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

            if (sessionState ==
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

        lv_obj_set_style_bg_color(
            newSessionButton,
            Theme::Accent(),
            0);

        lv_obj_set_style_bg_opa(
            newSessionButton,
            LV_OPA_COVER,
            0);

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

    lv_obj_set_style_bg_color(
        newSessionButton,
        Theme::Muted(),
        0);

    lv_obj_set_style_bg_opa(
        newSessionButton,
        LV_OPA_50,
        0);

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

    lv_obj_set_style_bg_color(
        networksButton,
        showingNetworks
            ? Theme::Accent()
            : Theme::Muted(),
        0);

    lv_obj_set_style_bg_opa(
        networksButton,
        showingNetworks
            ? LV_OPA_COVER
            : LV_OPA_30,
        0);

    lv_obj_set_style_bg_color(
        channelsButton,
        showingNetworks
            ? Theme::Muted()
            : Theme::Accent(),
        0);

    lv_obj_set_style_bg_opa(
        channelsButton,
        showingNetworks
            ? LV_OPA_30
            : LV_OPA_COVER,
        0);
}

void ScanScreen::ShowNetworkResults(
    uint8_t networkCount)
{
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
    const WiFiMeasurementSessionState sessionState =
        WiFiService::GetMeasurementSessionState();

    const WiFiMeasurementSummary &summary =
        WiFiService::GetMeasurementSummary();

    if (sessionState ==
            WiFiMeasurementSessionState::Complete &&
        summary.available)
    {
        AddCompletedSessionSummary(
            summary);

        for (uint8_t index = 0;
             index <
                 WiFiMeasurementSummary::
                     CandidateCapacity;
             ++index)
        {
            AddCandidateAssessmentRow(
                summary.candidates[index]);
        }

        AddSectionLabel(
            "FINAL SCAN CHANNEL DETAIL");

        for (uint8_t channel = 1;
             channel <
                 WiFiMeasurementSummary::
                     ChannelCapacity;
             ++channel)
        {
            const WiFiChannelInfo &info =
                summary.channels[channel];

            if (info.networkCount == 0)
            {
                continue;
            }

            AddChannelRow(
                info);
        }

        return;
    }

    const WiFiChannelRecommendation
        &recommendation =
            WiFiService::GetChannelRecommendation();

    AddRecommendationRow(
        recommendation);

    for (uint8_t index = 0;
         index <
             WiFiService::CandidateChannelCount;
         ++index)
    {
        const WiFiChannelAssessment *assessment =
            WiFiService::GetCandidateAssessment(
                index);

        if (assessment == nullptr)
        {
            continue;
        }

        AddCandidateAssessmentRow(
            *assessment);
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
         channel <=
             WiFiService::Max2_4GHzChannel;
         ++channel)
    {
        const WiFiChannelInfo *info =
            WiFiService::GetChannelInfo(
                channel);

        if (info == nullptr ||
            info->networkCount == 0)
        {
            continue;
        }

        AddChannelRow(*info);
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
        "%s\n%s   CH %u   %ld dBm   %s",
        ssid,
        SignalQualityToText(
            network.signalQuality),
        network.channel,
        static_cast<long>(network.rssi),
        SecurityToText(network.security));

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

    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return;
    }

    const bool started =
        WiFiService::StartScan();

    instance->refreshPending = true;

    if (!started &&
        WiFiService::GetState() !=
            WiFiScanState::Scanning)
    {
        Serial.println(
            "ScanScreen: Scan request failed");
    }
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

    const bool started =
        WiFiService::StartAutomaticMeasurementSession();

    instance->refreshPending = true;

    if (!started)
    {
        Serial.println(
            "ScanScreen: Automatic measurement "
            "session failed to start");
    }
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
