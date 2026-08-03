#pragma once

#include "../Core/Page.h"
#include "../Core/Messaging/MessageTypes.h"
#include "../Services/WiFi/WiFiTypes.h"

#include <lvgl.h>

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

    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *networkList = nullptr;

    bool refreshPending = true;

    WiFiScanState displayedState =
        WiFiScanState::Idle;

    uint8_t displayedNetworkCount = 0;

    void RefreshFromService();

    void AddMessageRow(
        const char *text);

    void AddNetworkRow(
        const WiFiNetworkInfo &network);

    static const char *SecurityToText(
        WiFiSecurity security);
};