#include "ToolsScreen.h"

#include "../Services/Storage/StorageService.h"
#include "../Services/WiFi/WiFiService.h"
#include "../UI/Theme.h"

#include <Arduino.h>
#include <cstdio>
#include <lvgl.h>

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
}

bool MeasurementBusy()
{
    return
        WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::IsAutomaticMeasurementSessionActive();
}
}

void ToolsScreen::CreateContent()
{
    lv_obj_t *parent = GetContentArea();

    if (!UsbStorageService::IsFeatureBuilt())
    {
        lv_obj_t *title = lv_label_create(parent);
        lv_label_set_text(title, "Tools");
        lv_obj_set_style_text_color(
            title,
            Theme::Accent(),
            0);
        lv_obj_set_style_text_font(
            title,
            &lv_font_montserrat_20,
            0);
        lv_obj_align(
            title,
            LV_ALIGN_TOP_LEFT,
            10,
            10);

        lv_obj_t *message = lv_label_create(parent);
        lv_label_set_text(
            message,
            "Tools module coming soon");
        lv_obj_set_style_text_color(
            message,
            Theme::Text(),
            0);
        lv_obj_align(
            message,
            LV_ALIGN_CENTER,
            0,
            0);
        return;
    }

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "USB Storage");
    lv_obj_set_style_text_color(title, Theme::Text(), 0);
    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

    statusLabel = lv_label_create(parent);
    lv_label_set_text(statusLabel, "Checking storage...");
    lv_obj_set_style_text_color(
        statusLabel,
        Theme::Text(),
        0);
    lv_obj_set_style_text_font(
        statusLabel,
        &lv_font_montserrat_16,
        0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 10, 40);

    detailLabel = lv_label_create(parent);
    lv_label_set_long_mode(
        detailLabel,
        LV_LABEL_LONG_WRAP);
    // The application content area is 516 x 158 in the 536 x 240
    // landscape layout. Keep explanatory text in the left column so the
    // USB control remains visible within the content area.
    lv_obj_set_width(detailLabel, 320);
    lv_obj_set_style_text_color(
        detailLabel,
        Theme::Muted(),
        0);
    lv_obj_align(detailLabel, LV_ALIGN_TOP_LEFT, 10, 68);

    usbButton = lv_btn_create(parent);
    ConfigureControlButton(usbButton);
    lv_obj_set_size(usbButton, 150, 40);

    // The previous 10.19B layout placed the button at y=170, below the
    // 158-pixel-high application content area. Anchor it to the visible
    // right side instead so it remains reachable in every USB state.
    lv_obj_align(usbButton, LV_ALIGN_RIGHT_MID, -12, 0);

    lv_obj_add_event_cb(
        usbButton,
        ToolsScreen::HandleUsbStorageButton,
        LV_EVENT_CLICKED,
        this);

    usbButtonLabel = lv_label_create(usbButton);
    lv_obj_set_style_text_color(
        usbButtonLabel,
        Theme::ControlText(),
        0);
    lv_label_set_text(usbButtonLabel, "USB Storage");
    lv_obj_center(usbButtonLabel);

    displayedState = UsbStorageState::Unavailable;
    displayedMeasurementBusy = false;
    refreshPending = true;
    lastActiveRefreshMs = 0;

    RefreshUsbStorageStatus();
}

void ToolsScreen::Update()
{
    if (!UsbStorageService::IsFeatureBuilt())
    {
        return;
    }

    const UsbStorageState state =
        UsbStorageService::GetState();

    const bool measurementBusy = MeasurementBusy();

    if (state != displayedState ||
        measurementBusy != displayedMeasurementBusy)
    {
        refreshPending = true;
    }

    if (state == UsbStorageState::Active &&
        millis() - lastActiveRefreshMs >= 1000)
    {
        refreshPending = true;
        lastActiveRefreshMs = millis();
    }

    if (refreshPending)
    {
        RefreshUsbStorageStatus();
    }
}

void ToolsScreen::HandleUsbStorageButton(
    lv_event_t *event)
{
    if (event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    ToolsScreen *screen =
        static_cast<ToolsScreen *>(
            lv_event_get_user_data(event));

    if (screen == nullptr)
    {
        return;
    }

    if (UsbStorageService::IsActive())
    {
        UsbStorageService::ExitReadOnlyMode();
        screen->refreshPending = true;
        return;
    }

    if (MeasurementBusy())
    {
        Serial.println(
            "ToolsScreen: USB Storage Mode blocked while Wi-Fi is busy");
        screen->refreshPending = true;
        return;
    }

    if (!UsbStorageService::EnterReadOnlyMode())
    {
        Serial.println(
            "ToolsScreen: USB Storage Mode could not start");
    }

    screen->refreshPending = true;
}

void ToolsScreen::RefreshUsbStorageStatus()
{
    if (statusLabel == nullptr ||
        detailLabel == nullptr ||
        usbButton == nullptr ||
        usbButtonLabel == nullptr)
    {
        return;
    }

    const UsbStorageState state =
        UsbStorageService::GetState();

    const bool measurementBusy = MeasurementBusy();

    switch (state)
    {
        case UsbStorageState::Ready:
            lv_label_set_text(
                statusLabel,
                "Ready");

            if (measurementBusy)
            {
                lv_label_set_text(
                    detailLabel,
                    "Wait for the current Wi-Fi scan or measurement "
                    "session to finish before exposing the SD card.");

                lv_obj_add_state(
                    usbButton,
                    LV_STATE_DISABLED);
            }
            else
            {
                lv_label_set_text(
                    detailLabel,
                    "Expose the SentinelOS SD card to Windows as "
                    "read-only USB storage. SentinelOS storage writes "
                    "are suspended while the drive is active.");

                lv_obj_clear_state(
                    usbButton,
                    LV_STATE_DISABLED);
            }

            lv_label_set_text(
                usbButtonLabel,
                "USB Storage");
            break;

        case UsbStorageState::Active:
        {
            char detail[192];

            std::snprintf(
                detail,
                sizeof(detail),
                "USB STORAGE ACTIVE\n"
                "Windows can read the SD card. Use Eject in Windows "
                "when finished.\nReads: %lu   Rejected writes: %lu",
                static_cast<unsigned long>(
                    UsbStorageService::GetReadRequestCount()),
                static_cast<unsigned long>(
                    UsbStorageService::GetRejectedWriteCount()));

            lv_label_set_text(
                statusLabel,
                "Connected to host");

            lv_label_set_text(
                detailLabel,
                detail);

            lv_obj_clear_state(
                usbButton,
                LV_STATE_DISABLED);

            lv_label_set_text(
                usbButtonLabel,
                "Disconnect");
            break;
        }

        case UsbStorageState::HostEjected:
            lv_label_set_text(
                statusLabel,
                "Safely ejected");

            lv_label_set_text(
                detailLabel,
                "Windows released the SD card. SentinelOS storage "
                "access is available again. Tap Reconnect to expose "
                "the card again.");

            lv_obj_clear_state(
                usbButton,
                LV_STATE_DISABLED);

            lv_label_set_text(
                usbButtonLabel,
                "Reconnect");
            break;

        case UsbStorageState::Unavailable:
        default:
            lv_label_set_text(
                statusLabel,
                UsbStorageService::IsFeatureBuilt()
                    ? "SD card unavailable"
                    : "USB storage test build inactive");

            lv_label_set_text(
                detailLabel,
                UsbStorageService::IsFeatureBuilt()
                    ? "SentinelOS could not prepare the SD card for USB storage."
                    : "Normal SentinelOS keeps the hardware USB CDC/JTAG "
                      "interface. Build the 10.19B USB Storage environment "
                      "to test integrated mass storage.");

            lv_obj_add_state(
                usbButton,
                LV_STATE_DISABLED);

            lv_label_set_text(
                usbButtonLabel,
                "Unavailable");
            break;
    }

    displayedState = state;
    displayedMeasurementBusy = measurementBusy;
    refreshPending = false;
}
