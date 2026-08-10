#pragma once

#include "../Core/Page.h"
#include "../Services/USB/UsbStorageService.h"

class ToolsScreen : public Page
{
public:
    void Update() override;

protected:
    void CreateContent() override;

private:
    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *detailLabel = nullptr;
    lv_obj_t *usbButton = nullptr;
    lv_obj_t *usbButtonLabel = nullptr;

    UsbStorageState displayedState =
        UsbStorageState::Unavailable;

    bool displayedMeasurementBusy = false;
    bool refreshPending = true;
    uint32_t lastActiveRefreshMs = 0;

    static void HandleUsbStorageButton(
        lv_event_t *event);

    void RefreshUsbStorageStatus();
};
