#pragma once

#include "../Core/Page.h"
#include "../Core/Messaging/MessageTypes.h"
#include "../Services/WiFi/WiFiTypes.h"

#include <lvgl.h>

enum class WiFiScanView : uint8_t
{
    Networks = 0,
    Channels
};

class ScanScreen : public Page
{
public:
    void Begin();
    void Update() override;

protected:
    void CreateContent() override;

private:
    static ScanScreen *instance;

    static void HandleMessage(
        const Message &message);

    static void HandleScanButton(
        lv_event_t *event);

    static void HandleNewSessionButton(
        lv_event_t *event);

    static void HandleNetworksButton(
        lv_event_t *event);

    static void HandleChannelsButton(
        lv_event_t *event);

    static void HandleHistoryButton(
        lv_event_t *event);

    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *networkList = nullptr;

    lv_obj_t *scanButton = nullptr;
    lv_obj_t *scanButtonLabel = nullptr;

    lv_obj_t *newSessionButton = nullptr;
    lv_obj_t *newSessionButtonLabel = nullptr;

    lv_obj_t *networksButton = nullptr;
    lv_obj_t *channelsButton = nullptr;

    lv_obj_t *historyButton = nullptr;
    lv_obj_t *historyButtonLabel = nullptr;

    bool refreshPending = true;

    WiFiScanState displayedState =
        WiFiScanState::Idle;

    WiFiMeasurementSessionState displayedSessionState =
        WiFiMeasurementSessionState::Idle;

    uint8_t displayedNetworkCount = 0;
    uint8_t displayedSavedSessionCount = 0;

    bool reviewingSavedSession = false;
    uint8_t selectedSavedSessionIndex = 0;

    WiFiScanView activeView =
        WiFiScanView::Networks;

    void RefreshFromService();

    void UpdateScanButton(
        WiFiScanState state,
        WiFiMeasurementSessionState sessionState);

    void UpdateNewSessionButton(
        WiFiScanState state,
        WiFiMeasurementSessionState sessionState);

    void UpdateViewButtons();
    void UpdateHistoryControls();

    void ShowNetworkResults(
        uint8_t networkCount);

    void ShowChannelResults();

    void ShowMeasurementSummary(
        const WiFiMeasurementSummary &summary);

    void AddSavedSessionHeader(
        uint32_t sessionId,
        uint32_t completedAtMs);

    void AddMessageRow(
        const char *text);

    void AddNetworkRow(
        const WiFiNetworkInfo &network);

    void AddRecommendationRow(
        const WiFiChannelRecommendation
            &recommendation);

    void AddCompletedSessionSummary(
        const WiFiMeasurementSummary &summary);

    void AddSectionLabel(
        const char *text);

    void AddCandidateAssessmentRow(
        const WiFiChannelAssessment &assessment);

    void AddChannelRow(
        const WiFiChannelInfo &channelInfo);

    static const char *SignalQualityToText(
        WiFiSignalQuality quality);

    static const char *CongestionLevelToText(
        WiFiCongestionLevel congestion);

    static const char *RecommendationConfidenceToText(
        WiFiRecommendationConfidence confidence);

    static const char *SecurityToText(
        WiFiSecurity security);
};
