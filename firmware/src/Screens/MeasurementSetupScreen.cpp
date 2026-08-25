#include <Arduino.h>
#include <cstring>
#include <esp_heap_caps.h>

#include "MeasurementSetupScreen.h"

#include "../Services/Survey/SiteSurveyService.h"
#include "../Managers/SiteSurveyManager.h"
#include "../Managers/NavigationManager.h"
#include "../Services/Storage/StorageService.h"
#include "../Services/Imaging/FloorPlanImageRenderer.h"
#include "../Services/Time/TimeService.h"
#include "../Services/WiFi/WiFiService.h"

static MeasurementSetupScreen *instance = nullptr;

void MeasurementSetupScreen::Show(
    MeasurementSetupAction action)
{
    instance = this;

    pendingAction = action;

    if (root != nullptr)
    {
        // Measurement Setup is modal. Keep page-swipe navigation locked
        // while it is visible so Floor Plan dragging cannot change the
        // page underneath the overlay.
        NavigationManager::SetGestureNavigationEnabled(false);
        return;
    }

    selectedSavedSurveyId = 0;
    selectedSavedSurveyCreatedEpoch = 0;
    selectedSavedPointId = 0;
    selectedSavedPointSiteSurveyId = 0;
    ClearPendingMapPosition();

    lv_obj_t *parent =
        lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    root =
        lv_obj_create(parent);

    // Measurement Setup is a modal workflow. Raw horizontal touch
    // gestures are also observed by InputManager for page navigation;
    // suppress those gestures while this overlay is active so panning a
    // Floor Plan cannot silently navigate the underlying page away from
    // Wi-Fi Scan.
    NavigationManager::SetGestureNavigationEnabled(false);

    lv_obj_set_pos(
        root,
        0,
        0);

    lv_obj_set_size(
        root,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        root,
        8,
        0);

    lv_obj_set_style_pad_row(
        root,
        2,
        0);

    lv_obj_set_flex_flow(
        root,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        root,
        LV_OBJ_FLAG_SCROLLABLE);

    // ------------------------------------------------------------
    // Title
    // ------------------------------------------------------------

    lv_obj_t *title =
        lv_label_create(root);

    lv_label_set_text(
        title,
        "MEASUREMENT SETUP");

    // ------------------------------------------------------------
    // Site Survey
    // ------------------------------------------------------------

    lv_obj_t *surveyLabel =
        lv_label_create(root);

    lv_label_set_text(
        surveyLabel,
        "Site Survey");

    siteSurveyTextArea =
        lv_textarea_create(root);

    lv_obj_add_event_cb(
        siteSurveyTextArea,
        MeasurementSetupScreen::HandleTextAreaFocus,
        LV_EVENT_FOCUSED,
        nullptr);

    lv_obj_set_width(
        siteSurveyTextArea,
        lv_pct(100));

    lv_obj_set_height(
        siteSurveyTextArea,
        34);

    lv_textarea_set_one_line(
        siteSurveyTextArea,
        true);

    lv_textarea_set_max_length(
        siteSurveyTextArea,
        SiteSurveyInfo::NameCapacity - 1);

    lv_textarea_set_placeholder_text(
        siteSurveyTextArea,
        "e.g. Hyatt Regency - Level 2");

    if (SiteSurveyService::HasActiveSurvey())
    {
        const SiteSurveyInfo &survey =
            SiteSurveyService::GetActiveSurvey();

        lv_textarea_set_text(
            siteSurveyTextArea,
            survey.name);
    }

    lv_obj_t *surveyActionRow =
        lv_obj_create(root);

    lv_obj_remove_style_all(
        surveyActionRow);

    lv_obj_set_width(
        surveyActionRow,
        lv_pct(100));

    lv_obj_set_height(
        surveyActionRow,
        32);

    lv_obj_set_flex_flow(
        surveyActionRow,
        LV_FLEX_FLOW_ROW);

    lv_obj_set_style_pad_column(
        surveyActionRow,
        6,
        0);

    lv_obj_clear_flag(
        surveyActionRow,
        LV_OBJ_FLAG_SCROLLABLE);

    if (StorageService::GetSavedSiteSurveyCount() > 0)
    {
        selectSavedSurveyButton =
            lv_btn_create(surveyActionRow);

        lv_obj_set_width(
            selectSavedSurveyButton,
            0);

        lv_obj_set_height(
            selectSavedSurveyButton,
            32);

        lv_obj_set_flex_grow(
            selectSavedSurveyButton,
            1);

        lv_obj_t *selectSavedSurveyLabel =
            lv_label_create(
                selectSavedSurveyButton);

        lv_label_set_text(
            selectSavedSurveyLabel,
            "Saved Survey");

        lv_obj_center(
            selectSavedSurveyLabel);

        lv_obj_add_event_cb(
            selectSavedSurveyButton,
            MeasurementSetupScreen::
                HandleSelectSavedSurveyButton,
            LV_EVENT_CLICKED,
            nullptr);
    }

    floorPlansButton =
        lv_btn_create(surveyActionRow);

    lv_obj_set_width(
        floorPlansButton,
        0);

    lv_obj_set_height(
        floorPlansButton,
        32);

    lv_obj_set_flex_grow(
        floorPlansButton,
        1);

    lv_obj_t *floorPlansLabel =
        lv_label_create(floorPlansButton);

    lv_label_set_text(
        floorPlansLabel,
        "Floor Plans");

    lv_obj_center(
        floorPlansLabel);

    lv_obj_add_event_cb(
        floorPlansButton,
        MeasurementSetupScreen::HandleFloorPlansButton,
        LV_EVENT_CLICKED,
        nullptr);

    RefreshFloorPlanButtonState();

    // ------------------------------------------------------------
    // Survey Point
    // ------------------------------------------------------------

    lv_obj_t *pointLabel =
        lv_label_create(root);

    lv_label_set_text(
        pointLabel,
        "Survey Point");

    lv_obj_t *pointRow =
        lv_obj_create(root);

    lv_obj_remove_style_all(
        pointRow);

    lv_obj_set_width(
        pointRow,
        lv_pct(100));

    lv_obj_set_height(
        pointRow,
        34);

    lv_obj_set_flex_flow(
        pointRow,
        LV_FLEX_FLOW_ROW);

    lv_obj_set_style_pad_column(
        pointRow,
        6,
        0);

    lv_obj_clear_flag(
        pointRow,
        LV_OBJ_FLAG_SCROLLABLE);

    surveyPointTextArea =
        lv_textarea_create(pointRow);

    lv_obj_add_event_cb(
        surveyPointTextArea,
        MeasurementSetupScreen::HandleTextAreaFocus,
        LV_EVENT_FOCUSED,
        nullptr);

    lv_obj_set_width(
        surveyPointTextArea,
        0);

    lv_obj_set_height(
        surveyPointTextArea,
        34);

    lv_obj_set_flex_grow(
        surveyPointTextArea,
        1);

    lv_textarea_set_one_line(
        surveyPointTextArea,
        true);

    lv_textarea_set_max_length(
        surveyPointTextArea,
        WiFiMeasurementSummary::SurveyPointCapacity - 1);

    lv_textarea_set_placeholder_text(
        surveyPointTextArea,
        "e.g. Conference Room");

    selectSavedPointButton =
        lv_btn_create(pointRow);

    lv_obj_set_width(
        selectSavedPointButton,
        112);

    lv_obj_set_height(
        selectSavedPointButton,
        34);

    lv_obj_t *selectSavedPointLabel =
        lv_label_create(
            selectSavedPointButton);

    lv_label_set_text(
        selectSavedPointLabel,
        "Saved Point");

    lv_obj_center(
        selectSavedPointLabel);

    lv_obj_add_event_cb(
        selectSavedPointButton,
        MeasurementSetupScreen::
            HandleSelectSavedPointButton,
        LV_EVENT_CLICKED,
        nullptr);

    RefreshSavedPointButtonState();

    // ------------------------------------------------------------
    // Buttons
    // ------------------------------------------------------------

    lv_obj_t *buttonRow =
        lv_obj_create(root);

    lv_obj_remove_style_all(
        buttonRow);

    lv_obj_set_width(
        buttonRow,
        lv_pct(100));

    lv_obj_set_height(
        buttonRow,
        34);

    lv_obj_set_flex_flow(
        buttonRow,
        LV_FLEX_FLOW_ROW);

    lv_obj_set_style_pad_column(
        buttonRow,
        8,
        0);

    lv_obj_clear_flag(
        buttonRow,
        LV_OBJ_FLAG_SCROLLABLE);

    if (SiteSurveyService::HasActiveSurvey())
    {
        closeSurveyButton =
            lv_btn_create(buttonRow);

        lv_obj_set_width(
            closeSurveyButton,
            0);

        lv_obj_set_height(
            closeSurveyButton,
            32);

        lv_obj_set_flex_grow(
            closeSurveyButton,
            1);

        lv_obj_t *closeSurveyLabel =
            lv_label_create(closeSurveyButton);

        lv_label_set_text(
            closeSurveyLabel,
            "Close Survey");

        lv_obj_center(
            closeSurveyLabel);

        lv_obj_add_event_cb(
            closeSurveyButton,
            MeasurementSetupScreen::HandleCloseSurveyButton,
            LV_EVENT_CLICKED,
            nullptr);
    }
    
    cancelButton =
        lv_btn_create(buttonRow);

    lv_obj_set_width(
        cancelButton,
        0);

    lv_obj_set_height(
        cancelButton,
        32);

    lv_obj_set_flex_grow(
        cancelButton,
        1);

    lv_obj_t *cancelLabel =
        lv_label_create(cancelButton);

    lv_label_set_text(
        cancelLabel,
        "Cancel");

    lv_obj_center(
        cancelLabel);

    lv_obj_add_event_cb(
        cancelButton,
        MeasurementSetupScreen::HandleCancelButton,
        LV_EVENT_CLICKED,
        nullptr);

    startButton =
        lv_btn_create(buttonRow);

    lv_obj_set_width(
        startButton,
        0);

    lv_obj_set_height(
        startButton,
        32);

    lv_obj_set_flex_grow(
        startButton,
        1);

    lv_obj_t *startLabel =
        lv_label_create(startButton);

    lv_label_set_text(
        startLabel,
        "Start");

    lv_obj_center(
        startLabel);

    lv_obj_add_event_cb(
        startButton,
        MeasurementSetupScreen::HandleStartButton,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_move_foreground(
        root);
}

void MeasurementSetupScreen::
HandleSelectSavedSurveyButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    instance->OpenSurveySelector();
}

void MeasurementSetupScreen::
HandleSavedSurveyButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));

    if (rawIndex > 0xFF)
    {
        return;
    }

    const uint8_t index =
        static_cast<uint8_t>(
            rawIndex);

    const StoredSiteSurveyIndex *survey =
        StorageService::
            GetSavedSiteSurveyIndex(index);

    if (survey == nullptr ||
        !survey->available)
    {
        return;
    }

    if (instance->siteSurveyTextArea != nullptr)
    {
        lv_textarea_set_text(
            instance->siteSurveyTextArea,
            survey->name);
    }

    instance->ClearSavedPointSelection(true);
    instance->ClearPendingMapPosition();

    instance->selectedSavedSurveyId =
        survey->surveyId;

    instance->selectedSavedSurveyCreatedEpoch =
        survey->createdEpoch;

    instance->RefreshSavedPointButtonState();
    instance->RefreshFloorPlanButtonState();

    Serial.printf(
        "MeasurementSetupScreen: Saved Site Survey "
        "%lu selected: %s\n",
        static_cast<unsigned long>(
            survey->surveyId),
        survey->name);

    instance->CloseSurveySelector();
}

void MeasurementSetupScreen::
HandleSurveySelectorCancel(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    instance->CloseSurveySelector();
}

void MeasurementSetupScreen::
HandleFloorPlansButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    instance->OpenFloorPlanSelector();
}

void MeasurementSetupScreen::
HandleFloorPlanImportButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));

    if (rawIndex > 0xFF)
    {
        return;
    }

    const uint8_t index =
        static_cast<uint8_t>(rawIndex);

    const FloorPlanImportImage *image =
        StorageService::GetFloorPlanImportImage(index);

    if (image == nullptr ||
        !image->available)
    {
        return;
    }

    const uint32_t surveyId =
        instance->GetPointContextSurveyId();

    if (surveyId == 0)
    {
        return;
    }

    char importPath[
        FloorPlanImportImage::PathCapacity] = {};

    char displayName[
        FloorPlanImportImage::NameCapacity] = {};

    std::strncpy(
        importPath,
        image->path,
        sizeof(importPath) - 1);

    std::strncpy(
        displayName,
        image->name,
        sizeof(displayName) - 1);

    uint32_t createdEpoch = 0;
    TimeService::GetEpochTime(createdEpoch);

    uint32_t floorPlanId = 0;

    if (!SiteSurveyManager::RegisterFloorPlanImport(
            surveyId,
            importPath,
            createdEpoch,
            floorPlanId))
    {
        Serial.printf(
            "MeasurementSetupScreen: Floor Plan import "
            "failed: %s\n",
            displayName);

        return;
    }

    Serial.printf(
        "MeasurementSetupScreen: Floor Plan %lu "
        "imported for Site Survey %lu: %s\n",
        static_cast<unsigned long>(floorPlanId),
        static_cast<unsigned long>(surveyId),
        displayName);

    lv_obj_t *button =
        lv_event_get_target(event);

    if (button != nullptr)
    {
        lv_obj_add_state(
            button,
            LV_STATE_DISABLED);

        lv_obj_t *label =
            lv_obj_get_child(button, 0);

        if (label != nullptr)
        {
            char text[80] = {};

            std::snprintf(
                text,
                sizeof(text),
                "Imported #%lu  %s",
                static_cast<unsigned long>(floorPlanId),
                displayName);

            lv_label_set_text(label, text);
        }
    }
}

void MeasurementSetupScreen::
HandleRegisteredFloorPlanButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));

    if (rawIndex > 0xFF)
    {
        return;
    }

    instance->OpenFloorPlanViewer(
        static_cast<uint8_t>(rawIndex));
}

void MeasurementSetupScreen::
HandleFloorPlanViewerClose(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->floorPlanPlacementMode)
    {
        instance->ExitFloorPlanPlacementMode();
        return;
    }

    instance->CloseFloorPlanViewer();
}

void MeasurementSetupScreen::
HandleFloorPlanPlacementButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->floorPlanPlacementMode)
    {
        instance->ConfirmFloorPlanPlacement();
    }
    else
    {
        instance->EnterFloorPlanPlacementMode();
    }
}

void MeasurementSetupScreen::
HandleFloorPlanCanvasTouch(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        !instance->floorPlanPlacementMode ||
        instance->floorPlanCanvas == nullptr)
    {
        return;
    }

    const lv_event_code_t code =
        lv_event_get_code(event);

    if (code == LV_EVENT_RELEASED ||
        code == LV_EVENT_PRESS_LOST)
    {
        instance->floorPlanDragActive = false;
        return;
    }

    lv_indev_t *indev =
        lv_indev_get_act();

    if (indev == nullptr)
    {
        return;
    }

    lv_point_t point{};
    lv_indev_get_point(indev, &point);

    if (code == LV_EVENT_PRESSED)
    {
        instance->floorPlanDragActive = true;
        instance->floorPlanLastTouchX = point.x;
        instance->floorPlanLastTouchY = point.y;
        return;
    }

    if (code == LV_EVENT_PRESSING &&
        instance->floorPlanDragActive)
    {
        const lv_coord_t deltaX =
            static_cast<lv_coord_t>(
                point.x - instance->floorPlanLastTouchX);

        const lv_coord_t deltaY =
            static_cast<lv_coord_t>(
                point.y - instance->floorPlanLastTouchY);

        instance->floorPlanLastTouchX = point.x;
        instance->floorPlanLastTouchY = point.y;

        instance->ClampFloorPlanCanvasPosition(
            static_cast<lv_coord_t>(
                lv_obj_get_x(instance->floorPlanCanvas) + deltaX),
            static_cast<lv_coord_t>(
                lv_obj_get_y(instance->floorPlanCanvas) + deltaY));

        instance->UpdateFloorPlanPlacementStatus();
    }
}

void MeasurementSetupScreen::
HandleFloorPlanSelectorClose(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    instance->CloseFloorPlanSelector();
}

void MeasurementSetupScreen::
HandleSelectSavedPointButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    instance->OpenPointSelector();
}

void MeasurementSetupScreen::
HandleSavedPointButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));

    if (rawIndex > 0xFF)
    {
        return;
    }

    const uint8_t index =
        static_cast<uint8_t>(rawIndex);

    const StoredSiteSurveyPointIndex *point =
        StorageService::GetSavedSiteSurveyPointIndex(index);

    if (point == nullptr ||
        !point->available)
    {
        return;
    }

    const uint32_t surveyId =
        instance->GetPointContextSurveyId();

    if (surveyId == 0 ||
        point->siteSurveyId != surveyId)
    {
        return;
    }

    if (instance->surveyPointTextArea != nullptr)
    {
        lv_textarea_set_text(
            instance->surveyPointTextArea,
            point->name);
    }

    instance->selectedSavedPointId = point->pointId;
    instance->selectedSavedPointSiteSurveyId =
        point->siteSurveyId;

    instance->ClearPendingMapPosition();

    if (point->floorPlanId != 0)
    {
        instance->pendingMapPositionValid = true;
        instance->pendingMapPositionPersisted = true;
        instance->pendingMapFloorPlanId = point->floorPlanId;
        instance->pendingMapX = point->mapX;
        instance->pendingMapY = point->mapY;
    }

    Serial.printf(
        "MeasurementSetupScreen: Saved Survey Point "
        "%lu selected for Site Survey %lu: %s\n",
        static_cast<unsigned long>(point->pointId),
        static_cast<unsigned long>(point->siteSurveyId),
        point->name);

    instance->ClosePointSelector();
}

void MeasurementSetupScreen::
HandlePointSelectorCancel(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    instance->ClosePointSelector();
}

void MeasurementSetupScreen::HandleCloseSurveyButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    if (!SiteSurveyService::HasActiveSurvey())
    {
        return;
    }

    if (!SiteSurveyManager::CloseSurvey())
    {
        Serial.println(
            "MeasurementSetupScreen: "
            "Site Survey could not be closed");

        return;
    }

    Serial.println(
        "MeasurementSetupScreen: "
        "Site Survey closed");

    instance->Hide();
}

void MeasurementSetupScreen::HandleCancelButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    instance->Hide();
}

void MeasurementSetupScreen::HandleStartButton(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr ||
        lv_event_get_code(event) !=
            LV_EVENT_CLICKED)
    {
        return;
    }

    if (instance->siteSurveyTextArea == nullptr ||
        instance->surveyPointTextArea == nullptr)
    {
        return;
    }

    const char *surveyName =
        lv_textarea_get_text(
            instance->siteSurveyTextArea);

    const char *surveyPoint =
        lv_textarea_get_text(
            instance->surveyPointTextArea);

    // Site Survey is required.
    if (surveyName == nullptr ||
        surveyName[0] == '\0')
    {
        instance->OpenTextEditor(
            instance->siteSurveyTextArea);

        return;
    }

    // Each measurement must identify its physical Survey Point.
    if (surveyPoint == nullptr ||
        surveyPoint[0] == '\0')
    {
        instance->OpenTextEditor(
            instance->surveyPointTextArea);

        return;
    }

    bool surveyPrepared = false;

    if (instance->selectedSavedSurveyId != 0)
    {
        // A saved survey retains its original identity and
        // creation timestamp when it is resumed.
        surveyPrepared =
            SiteSurveyManager::PrepareSavedSurvey(
                instance->selectedSavedSurveyId,
                surveyName,
                instance->selectedSavedSurveyCreatedEpoch);
    }
    else
    {
        uint32_t createdEpoch = 0;

        // Zero remains acceptable if wall-clock time is unavailable.
        TimeService::GetEpochTime(
            createdEpoch);

        surveyPrepared =
            SiteSurveyManager::PrepareSurvey(
                surveyName,
                createdEpoch);
    }

    if (!surveyPrepared)
    {
        Serial.println(
            "MeasurementSetupScreen: "
            "Site Survey preparation failed");

        return;
    }

    bool pointPrepared = false;

    if (instance->selectedSavedPointId != 0)
    {
        pointPrepared =
            SiteSurveyManager::PrepareSavedSurveyPoint(
                instance->selectedSavedPointId,
                instance->selectedSavedPointSiteSurveyId,
                surveyPoint);
    }
    else
    {
        uint32_t pointCreatedEpoch = 0;

        TimeService::GetEpochTime(
            pointCreatedEpoch);

        pointPrepared =
            SiteSurveyManager::PrepareNewSurveyPoint(
                surveyPoint,
                pointCreatedEpoch);
    }

    if (!pointPrepared)
    {
        Serial.println(
            "MeasurementSetupScreen: "
            "Survey Point preparation failed");

        return;
    }

    if (instance->pendingMapPositionValid &&
        !instance->pendingMapPositionPersisted)
    {
        const uint32_t preparedPointId =
            WiFiService::GetMeasurementSurveyPointId();

        if (preparedPointId == 0 ||
            !StorageService::SetSiteSurveyPointMapPosition(
                preparedPointId,
                instance->pendingMapFloorPlanId,
                instance->pendingMapX,
                instance->pendingMapY))
        {
            // PrepareNewSurveyPoint may already have created the Point.
            // Preserve that identity so a retry reuses it instead of
            // creating a duplicate Point after a storage failure.
            if (preparedPointId != 0 &&
                instance->selectedSavedPointId == 0)
            {
                instance->selectedSavedPointId =
                    preparedPointId;
                instance->selectedSavedPointSiteSurveyId =
                    instance->GetPointContextSurveyId();
            }

            Serial.println(
                "MeasurementSetupScreen: "
                "Survey Point map position could not be saved");

            return;
        }

        instance->pendingMapPositionPersisted = true;
    }

    const MeasurementSetupAction action =
        instance->pendingAction;

    instance->Hide();

    bool started = false;

    if (action ==
        MeasurementSetupAction::MeasurementSession)
    {
        started =
            WiFiService::
                StartAutomaticMeasurementSession();
    }
    else
    {
        started =
            WiFiService::StartScan();
    }

    if (!started)
    {
        Serial.println(
            "MeasurementSetupScreen: "
            "Measurement could not be started");
    }
}   

void MeasurementSetupScreen::HandleTextAreaFocus(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr)
    {
        return;
    }

    lv_obj_t *target =
        lv_event_get_target(event);

    if (target ==
        instance->siteSurveyTextArea)
    {
        // Manual editing means this is no longer an
        // explicit selection of a stored survey. Any
        // selected Point identity is also parent-specific.
        instance->selectedSavedSurveyId = 0;
        instance->selectedSavedSurveyCreatedEpoch = 0;
        instance->ClearSavedPointSelection(false);
        instance->ClearPendingMapPosition();
        instance->RefreshSavedPointButtonState();
        instance->RefreshFloorPlanButtonState();
    }
    else if (target ==
             instance->surveyPointTextArea)
    {
        // Editing a saved Point turns the value into a
        // deliberate new Point on Start.
        instance->ClearSavedPointSelection(false);
    }

    instance->OpenTextEditor(target);
}

void MeasurementSetupScreen::OpenSurveySelector()
{
    if (surveySelectorRoot != nullptr)
    {
        return;
    }

    const uint8_t surveyCount =
        StorageService::
            GetSavedSiteSurveyCount();

    if (surveyCount == 0)
    {
        return;
    }

    lv_obj_t *parent =
        lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    surveySelectorRoot =
        lv_obj_create(parent);

    lv_obj_set_pos(
        surveySelectorRoot,
        0,
        0);

    lv_obj_set_size(
        surveySelectorRoot,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        surveySelectorRoot,
        8,
        0);

    lv_obj_set_style_pad_row(
        surveySelectorRoot,
        6,
        0);

    lv_obj_set_flex_flow(
        surveySelectorRoot,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title =
        lv_label_create(
            surveySelectorRoot);

    lv_label_set_text(
        title,
        "SAVED SITE SURVEYS");

    lv_obj_t *surveyList =
        lv_obj_create(
            surveySelectorRoot);

    lv_obj_set_width(
        surveyList,
        lv_pct(100));

    lv_obj_set_height(
        surveyList,
        0);

    lv_obj_set_flex_grow(
        surveyList,
        1);

    lv_obj_add_flag(
        surveyList,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_scroll_dir(
        surveyList,
        LV_DIR_VER);

    lv_obj_set_scrollbar_mode(
        surveyList,
        LV_SCROLLBAR_MODE_AUTO);

    lv_obj_set_style_pad_all(
        surveyList,
        4,
        0);

    lv_obj_set_style_pad_row(
        surveyList,
        4,
        0);

    lv_obj_set_flex_flow(
        surveyList,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        surveySelectorRoot,
        LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t index = 0;
         index < surveyCount;
         ++index)
    {
        const StoredSiteSurveyIndex *survey =
            StorageService::
                GetSavedSiteSurveyIndex(index);

        if (survey == nullptr ||
            !survey->available)
        {
            continue;
        }

        lv_obj_t *button =
            lv_btn_create(
                surveyList);

        lv_obj_set_width(
            button,
            lv_pct(100));

        lv_obj_set_height(
            button,
            40);

        lv_obj_add_event_cb(
            button,
            MeasurementSetupScreen::
                HandleSavedSurveyButton,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(
                    index)));

        lv_obj_t *label =
            lv_label_create(button);

        lv_obj_set_width(
            label,
            lv_pct(100));

        lv_label_set_long_mode(
            label,
            LV_LABEL_LONG_DOT);

        char text[80];

        std::snprintf(
            text,
            sizeof(text),
            "#%lu  %s",
            static_cast<unsigned long>(
                survey->surveyId),
            survey->name);

        lv_label_set_text(
            label,
            text);

        lv_obj_center(
            label);
    }

    lv_obj_t *cancelButton =
        lv_btn_create(
            surveySelectorRoot);

    lv_obj_set_width(
        cancelButton,
        lv_pct(100));

    lv_obj_set_height(
        cancelButton,
        34);

    lv_obj_t *cancelLabel =
        lv_label_create(
            cancelButton);

    lv_label_set_text(
        cancelLabel,
        "Cancel");

    lv_obj_center(
        cancelLabel);

    lv_obj_add_event_cb(
        cancelButton,
        MeasurementSetupScreen::
            HandleSurveySelectorCancel,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_move_foreground(
        surveySelectorRoot);
}

void MeasurementSetupScreen::CloseSurveySelector()
{
    if (surveySelectorRoot == nullptr)
    {
        return;
    }

    lv_obj_add_flag(
        surveySelectorRoot,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_del_async(
        surveySelectorRoot);

    surveySelectorRoot = nullptr;
}

void MeasurementSetupScreen::OpenFloorPlanSelector()
{
    if (floorPlanSelectorRoot != nullptr)
    {
        return;
    }

    const uint32_t surveyId =
        GetPointContextSurveyId();

    if (surveyId == 0 ||
        !StorageService::IsAvailable() ||
        StorageService::IsExternalReadOnlyAccessActive())
    {
        return;
    }

    StorageService::RefreshFloorPlanImportCatalog();

    lv_obj_t *parent = lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    floorPlanSelectorRoot =
        lv_obj_create(parent);

    lv_obj_set_pos(
        floorPlanSelectorRoot,
        0,
        0);

    lv_obj_set_size(
        floorPlanSelectorRoot,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        floorPlanSelectorRoot,
        6,
        0);

    lv_obj_set_style_pad_row(
        floorPlanSelectorRoot,
        4,
        0);

    lv_obj_set_flex_flow(
        floorPlanSelectorRoot,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        floorPlanSelectorRoot,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title =
        lv_label_create(floorPlanSelectorRoot);

    lv_label_set_text(
        title,
        "FLOOR PLANS");

    lv_obj_t *surveyLabel =
        lv_label_create(floorPlanSelectorRoot);

    lv_obj_set_width(
        surveyLabel,
        lv_pct(100));

    lv_label_set_long_mode(
        surveyLabel,
        LV_LABEL_LONG_DOT);

    const char *surveyName =
        siteSurveyTextArea != nullptr
            ? lv_textarea_get_text(siteSurveyTextArea)
            : nullptr;

    char surveyText[96] = {};

    std::snprintf(
        surveyText,
        sizeof(surveyText),
        "Survey #%lu  %s",
        static_cast<unsigned long>(surveyId),
        surveyName != nullptr ? surveyName : "");

    lv_label_set_text(
        surveyLabel,
        surveyText);

    lv_obj_t *list =
        lv_obj_create(floorPlanSelectorRoot);

    lv_obj_set_width(
        list,
        lv_pct(100));

    lv_obj_set_height(
        list,
        0);

    lv_obj_set_flex_grow(
        list,
        1);

    lv_obj_add_flag(
        list,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_scroll_dir(
        list,
        LV_DIR_VER);

    lv_obj_set_scrollbar_mode(
        list,
        LV_SCROLLBAR_MODE_AUTO);

    lv_obj_set_style_pad_all(
        list,
        4,
        0);

    lv_obj_set_style_pad_row(
        list,
        4,
        0);

    lv_obj_set_flex_flow(
        list,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_t *registeredHeader =
        lv_label_create(list);

    lv_label_set_text(
        registeredHeader,
        "REGISTERED");

    bool hasRegisteredFloorPlan = false;

    const uint8_t savedCount =
        StorageService::GetSavedFloorPlanCount();

    for (uint8_t index = 0;
         index < savedCount;
         ++index)
    {
        const StoredFloorPlanIndex *floorPlan =
            StorageService::GetSavedFloorPlanIndex(index);

        if (floorPlan == nullptr ||
            !floorPlan->available ||
            floorPlan->siteSurveyId != surveyId)
        {
            continue;
        }

        hasRegisteredFloorPlan = true;

        lv_obj_t *row =
            lv_btn_create(list);

        lv_obj_set_width(
            row,
            lv_pct(100));

        lv_obj_set_height(
            row,
            38);

        lv_obj_set_style_pad_all(
            row,
            6,
            0);

        lv_obj_clear_flag(
            row,
            LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_add_event_cb(
            row,
            MeasurementSetupScreen::
                HandleRegisteredFloorPlanButton,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(index)));

        lv_obj_t *label =
            lv_label_create(row);

        lv_obj_set_width(
            label,
            lv_pct(100));

        lv_label_set_long_mode(
            label,
            LV_LABEL_LONG_DOT);

        char text[96] = {};

        if (floorPlan->sourceWidth != 0 &&
            floorPlan->sourceHeight != 0)
        {
            std::snprintf(
                text,
                sizeof(text),
                "View  #%lu  %s  %ux%u",
                static_cast<unsigned long>(
                    floorPlan->floorPlanId),
                floorPlan->name,
                static_cast<unsigned int>(
                    floorPlan->sourceWidth),
                static_cast<unsigned int>(
                    floorPlan->sourceHeight));
        }
        else
        {
            std::snprintf(
                text,
                sizeof(text),
                "View  #%lu  %s",
                static_cast<unsigned long>(
                    floorPlan->floorPlanId),
                floorPlan->name);
        }

        lv_label_set_text(label, text);
        lv_obj_center(label);
    }

    if (!hasRegisteredFloorPlan)
    {
        lv_obj_t *emptyLabel =
            lv_label_create(list);

        lv_label_set_text(
            emptyLabel,
            "No registered Floor Plans");
    }

    lv_obj_t *importHeader =
        lv_label_create(list);

    lv_label_set_text(
        importHeader,
        "IMPORT IMAGES");

    const uint8_t importCount =
        StorageService::GetFloorPlanImportCount();

    if (importCount == 0)
    {
        lv_obj_t *emptyImportLabel =
            lv_label_create(list);

        lv_obj_set_width(
            emptyImportLabel,
            lv_pct(100));

        lv_label_set_long_mode(
            emptyImportLabel,
            LV_LABEL_LONG_WRAP);

        lv_label_set_text(
            emptyImportLabel,
            "No JPG, PNG, or BMP images found. "
            "Use Tools > USB Transfer to copy files to "
            "/sentinel/import/floorplans/.");
    }
    else
    {
        for (uint8_t index = 0;
             index < importCount;
             ++index)
        {
            const FloorPlanImportImage *image =
                StorageService::GetFloorPlanImportImage(index);

            if (image == nullptr ||
                !image->available)
            {
                continue;
            }

            lv_obj_t *button =
                lv_btn_create(list);

            lv_obj_set_width(
                button,
                lv_pct(100));

            lv_obj_set_height(
                button,
                40);

            lv_obj_add_event_cb(
                button,
                MeasurementSetupScreen::
                    HandleFloorPlanImportButton,
                LV_EVENT_CLICKED,
                reinterpret_cast<void *>(
                    static_cast<uintptr_t>(index)));

            lv_obj_t *label =
                lv_label_create(button);

            lv_obj_set_width(
                label,
                lv_pct(100));

            lv_label_set_long_mode(
                label,
                LV_LABEL_LONG_DOT);

            char text[96] = {};

            if (image->sourceWidth != 0 &&
                image->sourceHeight != 0)
            {
                std::snprintf(
                    text,
                    sizeof(text),
                    "+ %s  %ux%u",
                    image->name,
                    static_cast<unsigned int>(
                        image->sourceWidth),
                    static_cast<unsigned int>(
                        image->sourceHeight));
            }
            else
            {
                std::snprintf(
                    text,
                    sizeof(text),
                    "+ %s",
                    image->name);
            }

            lv_label_set_text(label, text);
            lv_obj_center(label);
        }
    }

    lv_obj_t *closeButton =
        lv_btn_create(floorPlanSelectorRoot);

    lv_obj_set_width(
        closeButton,
        lv_pct(100));

    lv_obj_set_height(
        closeButton,
        34);

    lv_obj_t *closeLabel =
        lv_label_create(closeButton);

    lv_label_set_text(
        closeLabel,
        "Back to Setup");

    lv_obj_center(closeLabel);

    lv_obj_add_event_cb(
        closeButton,
        MeasurementSetupScreen::
            HandleFloorPlanSelectorClose,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_move_foreground(
        floorPlanSelectorRoot);
}

void MeasurementSetupScreen::CloseFloorPlanSelector()
{
    if (floorPlanSelectorRoot == nullptr)
    {
        return;
    }

    lv_obj_add_flag(
        floorPlanSelectorRoot,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_del_async(
        floorPlanSelectorRoot);

    floorPlanSelectorRoot = nullptr;
}

void MeasurementSetupScreen::OpenFloorPlanViewer(
    uint8_t savedFloorPlanIndex)
{
    if (floorPlanViewerRoot != nullptr ||
        StorageService::IsExternalReadOnlyAccessActive())
    {
        return;
    }

    const StoredFloorPlanIndex *floorPlan =
        StorageService::GetSavedFloorPlanIndex(
            savedFloorPlanIndex);

    const uint32_t surveyId =
        GetPointContextSurveyId();

    if (floorPlan == nullptr ||
        !floorPlan->available ||
        floorPlan->siteSurveyId == 0 ||
        floorPlan->siteSurveyId != surveyId ||
        floorPlan->imagePath[0] == '\0')
    {
        return;
    }

    lv_obj_t *parent = lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    floorPlanViewerFloorPlanId =
        floorPlan->floorPlanId;

    floorPlanPlacementMode = false;
    floorPlanDragActive = false;

    floorPlanViewerRoot =
        lv_obj_create(parent);

    lv_obj_set_pos(
        floorPlanViewerRoot,
        0,
        0);

    lv_obj_set_size(
        floorPlanViewerRoot,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        floorPlanViewerRoot,
        0,
        0);

    lv_obj_set_style_pad_row(
        floorPlanViewerRoot,
        0,
        0);

    lv_obj_set_style_bg_color(
        floorPlanViewerRoot,
        lv_color_make(0, 0, 0),
        0);

    lv_obj_set_style_border_width(
        floorPlanViewerRoot,
        0,
        0);

    lv_obj_set_flex_flow(
        floorPlanViewerRoot,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        floorPlanViewerRoot,
        LV_OBJ_FLAG_SCROLLABLE);

    // ------------------------------------------------------------
    // Compact viewer header
    // ------------------------------------------------------------

    lv_obj_t *header =
        lv_obj_create(floorPlanViewerRoot);

    lv_obj_set_width(
        header,
        lv_pct(100));

    lv_obj_set_height(
        header,
        30);

    lv_obj_set_style_pad_all(
        header,
        2,
        0);

    lv_obj_set_style_pad_column(
        header,
        4,
        0);

    lv_obj_set_style_border_width(
        header,
        0,
        0);

    lv_obj_set_flex_flow(
        header,
        LV_FLEX_FLOW_ROW);

    lv_obj_clear_flag(
        header,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *backButton =
        lv_btn_create(header);

    lv_obj_set_width(
        backButton,
        58);

    lv_obj_set_height(
        backButton,
        26);

    floorPlanViewerBackLabel =
        lv_label_create(backButton);

    lv_label_set_text(
        floorPlanViewerBackLabel,
        "Back");

    lv_obj_center(
        floorPlanViewerBackLabel);

    lv_obj_add_event_cb(
        backButton,
        MeasurementSetupScreen::
            HandleFloorPlanViewerClose,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_t *title =
        lv_label_create(header);

    lv_obj_set_width(
        title,
        0);

    lv_obj_set_flex_grow(
        title,
        1);

    lv_label_set_long_mode(
        title,
        LV_LABEL_LONG_DOT);

    char titleText[96] = {};

    std::snprintf(
        titleText,
        sizeof(titleText),
        "#%lu  %s",
        static_cast<unsigned long>(
            floorPlan->floorPlanId),
        floorPlan->name);

    lv_label_set_text(
        title,
        titleText);

    floorPlanPlacementButton =
        lv_btn_create(header);

    lv_obj_set_width(
        floorPlanPlacementButton,
        62);

    lv_obj_set_height(
        floorPlanPlacementButton,
        26);

    floorPlanPlacementButtonLabel =
        lv_label_create(
            floorPlanPlacementButton);

    lv_label_set_text(
        floorPlanPlacementButtonLabel,
        "Place");

    lv_obj_center(
        floorPlanPlacementButtonLabel);

    lv_obj_add_event_cb(
        floorPlanPlacementButton,
        MeasurementSetupScreen::
            HandleFloorPlanPlacementButton,
        LV_EVENT_CLICKED,
        nullptr);

    // A point name is required before a map position can be assigned.
    const char *pointName =
        surveyPointTextArea != nullptr
            ? lv_textarea_get_text(
                  surveyPointTextArea)
            : nullptr;

    if (pointName == nullptr ||
        pointName[0] == '\0')
    {
        lv_obj_add_state(
            floorPlanPlacementButton,
            LV_STATE_DISABLED);
    }

    // ------------------------------------------------------------
    // Full-width Floor Plan viewport
    // ------------------------------------------------------------

    floorPlanViewport =
        lv_obj_create(floorPlanViewerRoot);

    lv_obj_remove_style_all(
        floorPlanViewport);

    lv_obj_set_width(
        floorPlanViewport,
        lv_pct(100));

    lv_obj_set_height(
        floorPlanViewport,
        0);

    lv_obj_set_flex_grow(
        floorPlanViewport,
        1);

    lv_obj_set_style_bg_color(
        floorPlanViewport,
        lv_color_make(0, 0, 0),
        0);

    lv_obj_set_style_bg_opa(
        floorPlanViewport,
        LV_OPA_COVER,
        0);

    lv_obj_clear_flag(
        floorPlanViewport,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_update_layout(
        floorPlanViewerRoot);

    const lv_coord_t viewportWidth =
        lv_obj_get_width(
            floorPlanViewport);

    const lv_coord_t viewportHeight =
        lv_obj_get_height(
            floorPlanViewport);

    if (viewportWidth <= 0 ||
        viewportHeight <= 0)
    {
        lv_obj_t *errorLabel =
            lv_label_create(
                floorPlanViewport);

        lv_label_set_text(
            errorLabel,
            "Floor Plan viewport unavailable");

        lv_obj_center(errorLabel);
        lv_obj_move_foreground(
            floorPlanViewerRoot);
        return;
    }

    floorPlanViewportWidth =
        static_cast<uint16_t>(
            viewportWidth);

    floorPlanViewportHeight =
        static_cast<uint16_t>(
            viewportHeight);

    const size_t canvasBytes =
        static_cast<size_t>(viewportWidth) *
        static_cast<size_t>(viewportHeight) *
        sizeof(lv_color_t);

    floorPlanCanvasBuffer =
        static_cast<lv_color_t *>(
            heap_caps_malloc(
                canvasBytes,
                MALLOC_CAP_SPIRAM |
                    MALLOC_CAP_8BIT));

    if (floorPlanCanvasBuffer == nullptr)
    {
        floorPlanCanvasBuffer =
            static_cast<lv_color_t *>(
                heap_caps_malloc(
                    canvasBytes,
                    MALLOC_CAP_8BIT));
    }

    if (floorPlanCanvasBuffer == nullptr)
    {
        lv_obj_t *errorLabel =
            lv_label_create(
                floorPlanViewport);

        lv_label_set_text(
            errorLabel,
            "Not enough memory to open Floor Plan");

        lv_obj_center(errorLabel);
        lv_obj_move_foreground(
            floorPlanViewerRoot);
        return;
    }

    floorPlanCanvas =
        lv_canvas_create(
            floorPlanViewport);

    lv_canvas_set_buffer(
        floorPlanCanvas,
        floorPlanCanvasBuffer,
        viewportWidth,
        viewportHeight,
        LV_IMG_CF_TRUE_COLOR);

    lv_obj_set_pos(
        floorPlanCanvas,
        0,
        0);

    FloorPlanRenderInfo renderInfo{};

    if (!FloorPlanImageRenderer::RenderFit(
            floorPlan->imagePath,
            floorPlanCanvasBuffer,
            static_cast<uint16_t>(
                viewportWidth),
            static_cast<uint16_t>(
                viewportHeight),
            renderInfo))
    {
        lv_obj_add_state(
            floorPlanPlacementButton,
            LV_STATE_DISABLED);

        lv_obj_t *errorLabel =
            lv_label_create(
                floorPlanViewport);

        lv_obj_set_width(
            errorLabel,
            lv_pct(90));

        lv_label_set_long_mode(
            errorLabel,
            LV_LABEL_LONG_WRAP);

        char errorText[128] = {};

        std::snprintf(
            errorText,
            sizeof(errorText),
            "Unable to display Floor Plan\n%s",
            FloorPlanImageRenderer::GetLastError());

        lv_label_set_text(
            errorLabel,
            errorText);

        lv_obj_center(errorLabel);

        Serial.printf(
            "MeasurementSetupScreen: Floor Plan %lu viewer failed: %s\n",
            static_cast<unsigned long>(
                floorPlan->floorPlanId),
            FloorPlanImageRenderer::GetLastError());
    }
    else
    {
        floorPlanRenderedWidth =
            renderInfo.renderedWidth;

        floorPlanRenderedHeight =
            renderInfo.renderedHeight;

        floorPlanRenderedOffsetX =
            renderInfo.offsetX;

        floorPlanRenderedOffsetY =
            renderInfo.offsetY;

        // Render every persistent Survey Point mapped to this Floor Plan.
        // Markers are children of the canvas so they automatically follow
        // the Floor Plan while it is panned beneath the fixed crosshair.
        RenderFloorPlanPointMarkers(
            surveyId);

        lv_obj_add_flag(
            floorPlanCanvas,
            LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(
            floorPlanCanvas,
            MeasurementSetupScreen::
                HandleFloorPlanCanvasTouch,
            LV_EVENT_PRESSED,
            nullptr);

        lv_obj_add_event_cb(
            floorPlanCanvas,
            MeasurementSetupScreen::
                HandleFloorPlanCanvasTouch,
            LV_EVENT_PRESSING,
            nullptr);

        lv_obj_add_event_cb(
            floorPlanCanvas,
            MeasurementSetupScreen::
                HandleFloorPlanCanvasTouch,
            LV_EVENT_RELEASED,
            nullptr);

        lv_obj_add_event_cb(
            floorPlanCanvas,
            MeasurementSetupScreen::
                HandleFloorPlanCanvasTouch,
            LV_EVENT_PRESS_LOST,
            nullptr);

        lv_obj_invalidate(
            floorPlanCanvas);

        // --------------------------------------------------------
        // Fixed center crosshair. The map moves underneath it.
        // The black outer strokes plus white inner strokes keep the
        // reticle visible on both bright and dark Floor Plans.
        // --------------------------------------------------------

        floorPlanCrosshairRoot =
            lv_obj_create(
                floorPlanViewport);

        lv_obj_remove_style_all(
            floorPlanCrosshairRoot);

        lv_obj_set_size(
            floorPlanCrosshairRoot,
            35,
            35);

        lv_obj_center(
            floorPlanCrosshairRoot);

        lv_obj_clear_flag(
            floorPlanCrosshairRoot,
            LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *horizontalOuter =
            lv_obj_create(
                floorPlanCrosshairRoot);

        lv_obj_remove_style_all(
            horizontalOuter);

        lv_obj_set_size(
            horizontalOuter,
            35,
            3);

        lv_obj_set_style_bg_color(
            horizontalOuter,
            lv_color_make(0, 0, 0),
            0);

        lv_obj_set_style_bg_opa(
            horizontalOuter,
            LV_OPA_COVER,
            0);

        lv_obj_center(
            horizontalOuter);

        lv_obj_clear_flag(
            horizontalOuter,
            LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *horizontalInner =
            lv_obj_create(
                floorPlanCrosshairRoot);

        lv_obj_remove_style_all(
            horizontalInner);

        lv_obj_set_size(
            horizontalInner,
            35,
            1);

        lv_obj_set_style_bg_color(
            horizontalInner,
            lv_color_make(255, 255, 255),
            0);

        lv_obj_set_style_bg_opa(
            horizontalInner,
            LV_OPA_COVER,
            0);

        lv_obj_center(
            horizontalInner);

        lv_obj_clear_flag(
            horizontalInner,
            LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *verticalOuter =
            lv_obj_create(
                floorPlanCrosshairRoot);

        lv_obj_remove_style_all(
            verticalOuter);

        lv_obj_set_size(
            verticalOuter,
            3,
            35);

        lv_obj_set_style_bg_color(
            verticalOuter,
            lv_color_make(0, 0, 0),
            0);

        lv_obj_set_style_bg_opa(
            verticalOuter,
            LV_OPA_COVER,
            0);

        lv_obj_center(
            verticalOuter);

        lv_obj_clear_flag(
            verticalOuter,
            LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *verticalInner =
            lv_obj_create(
                floorPlanCrosshairRoot);

        lv_obj_remove_style_all(
            verticalInner);

        lv_obj_set_size(
            verticalInner,
            1,
            35);

        lv_obj_set_style_bg_color(
            verticalInner,
            lv_color_make(255, 255, 255),
            0);

        lv_obj_set_style_bg_opa(
            verticalInner,
            LV_OPA_COVER,
            0);

        lv_obj_center(
            verticalInner);

        lv_obj_clear_flag(
            verticalInner,
            LV_OBJ_FLAG_CLICKABLE);

        floorPlanPlacementStatusLabel =
            lv_label_create(
                floorPlanViewport);

        lv_obj_set_width(
            floorPlanPlacementStatusLabel,
            lv_pct(96));

        lv_label_set_long_mode(
            floorPlanPlacementStatusLabel,
            LV_LABEL_LONG_DOT);

        lv_obj_set_style_text_align(
            floorPlanPlacementStatusLabel,
            LV_TEXT_ALIGN_CENTER,
            0);

        // Force high-contrast status text. The screen theme can otherwise
        // inherit black label text, which disappears over the Floor Plan.
        lv_obj_set_style_text_color(
            floorPlanPlacementStatusLabel,
            lv_color_make(255, 255, 255),
            0);

        lv_obj_set_style_bg_color(
            floorPlanPlacementStatusLabel,
            lv_color_make(0, 0, 0),
            0);

        lv_obj_set_style_bg_opa(
            floorPlanPlacementStatusLabel,
            LV_OPA_70,
            0);

        lv_obj_set_style_pad_all(
            floorPlanPlacementStatusLabel,
            2,
            0);

        lv_obj_align(
            floorPlanPlacementStatusLabel,
            LV_ALIGN_BOTTOM_MID,
            0,
            -2);

        lv_obj_add_flag(
            floorPlanCrosshairRoot,
            LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_flag(
            floorPlanPlacementStatusLabel,
            LV_OBJ_FLAG_HIDDEN);

        Serial.printf(
            "MeasurementSetupScreen: Viewing Floor Plan %lu: %s\n",
            static_cast<unsigned long>(
                floorPlan->floorPlanId),
            floorPlan->imagePath);
    }

    lv_obj_move_foreground(
        floorPlanViewerRoot);

    // If the selected Point already has a saved (or pending) position on
    // this Floor Plan, reopen directly at that position. This makes the
    // stored location immediately visible instead of showing the generic
    // fit-to-screen centre and requiring another Place tap.
    if (pendingMapPositionValid &&
        pendingMapFloorPlanId == floorPlanViewerFloorPlanId)
    {
        EnterFloorPlanPlacementMode();
    }
}

void MeasurementSetupScreen::CloseFloorPlanViewer()
{
    floorPlanPlacementMode = false;
    floorPlanDragActive = false;

    if (floorPlanViewerRoot != nullptr)
    {
        // The canvas uses an externally allocated PSRAM buffer, so delete
        // the LVGL object synchronously before releasing the buffer.
        lv_obj_del(
            floorPlanViewerRoot);

        floorPlanViewerRoot = nullptr;
    }

    floorPlanViewport = nullptr;
    floorPlanCanvas = nullptr;
    floorPlanViewerBackLabel = nullptr;
    floorPlanPlacementButton = nullptr;
    floorPlanPlacementButtonLabel = nullptr;
    floorPlanCrosshairRoot = nullptr;
    floorPlanPlacementStatusLabel = nullptr;

    floorPlanViewerFloorPlanId = 0;
    floorPlanViewportWidth = 0;
    floorPlanViewportHeight = 0;
    floorPlanRenderedWidth = 0;
    floorPlanRenderedHeight = 0;
    floorPlanRenderedOffsetX = 0;
    floorPlanRenderedOffsetY = 0;

    if (floorPlanCanvasBuffer != nullptr)
    {
        heap_caps_free(
            floorPlanCanvasBuffer);

        floorPlanCanvasBuffer = nullptr;
    }
}

bool MeasurementSetupScreen::EnterFloorPlanPlacementMode()
{
    if (floorPlanPlacementMode ||
        floorPlanViewerFloorPlanId == 0 ||
        floorPlanCanvas == nullptr ||
        floorPlanCrosshairRoot == nullptr ||
        floorPlanPlacementStatusLabel == nullptr ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0 ||
        surveyPointTextArea == nullptr)
    {
        return false;
    }

    const char *pointName =
        lv_textarea_get_text(
            surveyPointTextArea);

    if (pointName == nullptr ||
        pointName[0] == '\0')
    {
        return false;
    }

    floorPlanPlacementMode = true;
    floorPlanDragActive = false;

    if (floorPlanViewerBackLabel != nullptr)
    {
        lv_label_set_text(
            floorPlanViewerBackLabel,
            "Cancel");
    }

    if (floorPlanPlacementButtonLabel != nullptr)
    {
        lv_label_set_text(
            floorPlanPlacementButtonLabel,
            "Save");
    }

    lv_obj_clear_flag(
        floorPlanCrosshairRoot,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(
        floorPlanPlacementStatusLabel,
        LV_OBJ_FLAG_HIDDEN);

    if (pendingMapPositionValid &&
        pendingMapFloorPlanId ==
            floorPlanViewerFloorPlanId)
    {
        PositionFloorPlanCanvasAtMapPoint(
            pendingMapX,
            pendingMapY);
    }
    else
    {
        // An unmapped Point begins at the center of the Floor Plan.
        PositionFloorPlanCanvasAtMapPoint(
            StoredSiteSurveyPoint::MapCoordinateMaximum / 2,
            StoredSiteSurveyPoint::MapCoordinateMaximum / 2);
    }

    UpdateFloorPlanPlacementStatus();

    lv_obj_move_foreground(
        floorPlanCrosshairRoot);

    lv_obj_move_foreground(
        floorPlanPlacementStatusLabel);

    return true;
}

void MeasurementSetupScreen::ExitFloorPlanPlacementMode()
{
    if (!floorPlanPlacementMode)
    {
        return;
    }

    floorPlanPlacementMode = false;
    floorPlanDragActive = false;

    if (floorPlanViewerBackLabel != nullptr)
    {
        lv_label_set_text(
            floorPlanViewerBackLabel,
            "Back");
    }

    if (floorPlanPlacementButtonLabel != nullptr)
    {
        lv_label_set_text(
            floorPlanPlacementButtonLabel,
            "Place");
    }

    if (floorPlanCrosshairRoot != nullptr)
    {
        lv_obj_add_flag(
            floorPlanCrosshairRoot,
            LV_OBJ_FLAG_HIDDEN);
    }

    if (floorPlanPlacementStatusLabel != nullptr)
    {
        lv_obj_add_flag(
            floorPlanPlacementStatusLabel,
            LV_OBJ_FLAG_HIDDEN);
    }

    if (floorPlanCanvas != nullptr)
    {
        // Return to the validated 10.24D fit-to-screen view.
        lv_obj_set_pos(
            floorPlanCanvas,
            0,
            0);
    }
}

bool MeasurementSetupScreen::ConfirmFloorPlanPlacement()
{
    if (!floorPlanPlacementMode ||
        floorPlanViewerFloorPlanId == 0)
    {
        return false;
    }

    uint16_t mapX = 0;
    uint16_t mapY = 0;

    if (!GetFloorPlanCrosshairMapPosition(
            mapX,
            mapY))
    {
        return false;
    }

    bool persisted = false;

    if (selectedSavedPointId != 0)
    {
        const uint32_t surveyId =
            GetPointContextSurveyId();

        if (surveyId == 0 ||
            selectedSavedPointSiteSurveyId != surveyId ||
            !StorageService::SetSiteSurveyPointMapPosition(
                selectedSavedPointId,
                floorPlanViewerFloorPlanId,
                mapX,
                mapY))
        {
            Serial.println(
                "MeasurementSetupScreen: Saved Survey Point "
                "map placement failed");
            return false;
        }

        persisted = true;
    }

    pendingMapPositionValid = true;
    pendingMapPositionPersisted = persisted;
    pendingMapFloorPlanId =
        floorPlanViewerFloorPlanId;
    pendingMapX = mapX;
    pendingMapY = mapY;

    const char *pointName =
        surveyPointTextArea != nullptr
            ? lv_textarea_get_text(
                  surveyPointTextArea)
            : "";

    Serial.printf(
        "MeasurementSetupScreen: Survey Point %s %s "
        "Floor Plan %lu at %u,%u\n",
        pointName != nullptr ? pointName : "",
        persisted ? "mapped to" : "placement pending on",
        static_cast<unsigned long>(
            floorPlanViewerFloorPlanId),
        static_cast<unsigned int>(mapX),
        static_cast<unsigned int>(mapY));

    // Saving a Point placement completes the Floor Plan task. Return
    // directly to Measurement Setup instead of leaving the user to back
    // through the viewer and Floor Plan selector manually.
    ExitFloorPlanPlacementMode();
    CloseFloorPlanViewer();
    CloseFloorPlanSelector();
    return true;
}

void MeasurementSetupScreen::ClampFloorPlanCanvasPosition(
    lv_coord_t requestedX,
    lv_coord_t requestedY)
{
    if (floorPlanCanvas == nullptr ||
        floorPlanViewportWidth == 0 ||
        floorPlanViewportHeight == 0 ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0)
    {
        return;
    }

    const int32_t centerX =
        static_cast<int32_t>(
            floorPlanViewportWidth / 2U);

    const int32_t centerY =
        static_cast<int32_t>(
            floorPlanViewportHeight / 2U);

    const int32_t minimumX =
        centerX -
        (static_cast<int32_t>(
             floorPlanRenderedOffsetX) +
         static_cast<int32_t>(
             floorPlanRenderedWidth) - 1);

    const int32_t maximumX =
        centerX -
        static_cast<int32_t>(
            floorPlanRenderedOffsetX);

    const int32_t minimumY =
        centerY -
        (static_cast<int32_t>(
             floorPlanRenderedOffsetY) +
         static_cast<int32_t>(
             floorPlanRenderedHeight) - 1);

    const int32_t maximumY =
        centerY -
        static_cast<int32_t>(
            floorPlanRenderedOffsetY);

    int32_t clampedX = requestedX;
    int32_t clampedY = requestedY;

    if (clampedX < minimumX)
    {
        clampedX = minimumX;
    }
    else if (clampedX > maximumX)
    {
        clampedX = maximumX;
    }

    if (clampedY < minimumY)
    {
        clampedY = minimumY;
    }
    else if (clampedY > maximumY)
    {
        clampedY = maximumY;
    }

    lv_obj_set_pos(
        floorPlanCanvas,
        static_cast<lv_coord_t>(clampedX),
        static_cast<lv_coord_t>(clampedY));
}

void MeasurementSetupScreen::PositionFloorPlanCanvasAtMapPoint(
    uint16_t mapX,
    uint16_t mapY)
{
    if (floorPlanCanvas == nullptr ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0)
    {
        return;
    }

    if (mapX > StoredSiteSurveyPoint::MapCoordinateMaximum)
    {
        mapX = StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    if (mapY > StoredSiteSurveyPoint::MapCoordinateMaximum)
    {
        mapY = StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    uint32_t renderedX = 0;
    uint32_t renderedY = 0;

    if (floorPlanRenderedWidth > 1)
    {
        renderedX =
            (static_cast<uint32_t>(mapX) *
                 (floorPlanRenderedWidth - 1U) +
             StoredSiteSurveyPoint::MapCoordinateMaximum / 2U) /
            StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    if (floorPlanRenderedHeight > 1)
    {
        renderedY =
            (static_cast<uint32_t>(mapY) *
                 (floorPlanRenderedHeight - 1U) +
             StoredSiteSurveyPoint::MapCoordinateMaximum / 2U) /
            StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    const int32_t requestedX =
        static_cast<int32_t>(
            floorPlanViewportWidth / 2U) -
        static_cast<int32_t>(
            floorPlanRenderedOffsetX) -
        static_cast<int32_t>(renderedX);

    const int32_t requestedY =
        static_cast<int32_t>(
            floorPlanViewportHeight / 2U) -
        static_cast<int32_t>(
            floorPlanRenderedOffsetY) -
        static_cast<int32_t>(renderedY);

    ClampFloorPlanCanvasPosition(
        static_cast<lv_coord_t>(requestedX),
        static_cast<lv_coord_t>(requestedY));
}

bool MeasurementSetupScreen::GetFloorPlanCrosshairMapPosition(
    uint16_t &mapX,
    uint16_t &mapY) const
{
    mapX = 0;
    mapY = 0;

    if (floorPlanCanvas == nullptr ||
        floorPlanViewportWidth == 0 ||
        floorPlanViewportHeight == 0 ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0)
    {
        return false;
    }

    const int32_t canvasX =
        lv_obj_get_x(
            floorPlanCanvas);

    const int32_t canvasY =
        lv_obj_get_y(
            floorPlanCanvas);

    int32_t renderedX =
        static_cast<int32_t>(
            floorPlanViewportWidth / 2U) -
        canvasX -
        static_cast<int32_t>(
            floorPlanRenderedOffsetX);

    int32_t renderedY =
        static_cast<int32_t>(
            floorPlanViewportHeight / 2U) -
        canvasY -
        static_cast<int32_t>(
            floorPlanRenderedOffsetY);

    if (renderedX < 0)
    {
        renderedX = 0;
    }
    else if (renderedX >=
             floorPlanRenderedWidth)
    {
        renderedX =
            floorPlanRenderedWidth - 1U;
    }

    if (renderedY < 0)
    {
        renderedY = 0;
    }
    else if (renderedY >=
             floorPlanRenderedHeight)
    {
        renderedY =
            floorPlanRenderedHeight - 1U;
    }

    if (floorPlanRenderedWidth > 1)
    {
        mapX =
            static_cast<uint16_t>(
                (static_cast<uint32_t>(renderedX) *
                     StoredSiteSurveyPoint::MapCoordinateMaximum +
                 (floorPlanRenderedWidth - 1U) / 2U) /
                (floorPlanRenderedWidth - 1U));
    }

    if (floorPlanRenderedHeight > 1)
    {
        mapY =
            static_cast<uint16_t>(
                (static_cast<uint32_t>(renderedY) *
                     StoredSiteSurveyPoint::MapCoordinateMaximum +
                 (floorPlanRenderedHeight - 1U) / 2U) /
                (floorPlanRenderedHeight - 1U));
    }

    return true;
}

void MeasurementSetupScreen::UpdateFloorPlanPlacementStatus()
{
    if (!floorPlanPlacementMode ||
        floorPlanPlacementStatusLabel == nullptr)
    {
        return;
    }

    uint16_t mapX = 0;
    uint16_t mapY = 0;

    if (!GetFloorPlanCrosshairMapPosition(
            mapX,
            mapY))
    {
        return;
    }

    const char *pointName =
        surveyPointTextArea != nullptr
            ? lv_textarea_get_text(
                  surveyPointTextArea)
            : "";

    char status[128] = {};

    std::snprintf(
        status,
        sizeof(status),
        "%s  X %u.%02u%%  Y %u.%02u%%",
        pointName != nullptr ? pointName : "",
        static_cast<unsigned int>(mapX / 100U),
        static_cast<unsigned int>(mapX % 100U),
        static_cast<unsigned int>(mapY / 100U),
        static_cast<unsigned int>(mapY % 100U));

    lv_label_set_text(
        floorPlanPlacementStatusLabel,
        status);
}

void MeasurementSetupScreen::RenderFloorPlanPointMarkers(
    uint32_t siteSurveyId)
{
    if (floorPlanCanvas == nullptr ||
        floorPlanViewerFloorPlanId == 0 ||
        siteSurveyId == 0 ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0)
    {
        return;
    }

    const uint8_t count =
        StorageService::GetSavedSiteSurveyPointCount();

    uint16_t mappedPointCount = 0;

    for (uint8_t index = 0;
         index < count;
         ++index)
    {
        const StoredSiteSurveyPointIndex *point =
            StorageService::GetSavedSiteSurveyPointIndex(index);

        if (point != nullptr &&
            point->available &&
            point->siteSurveyId == siteSurveyId &&
            point->floorPlanId == floorPlanViewerFloorPlanId &&
            point->mapX <= StoredSiteSurveyPoint::MapCoordinateMaximum &&
            point->mapY <= StoredSiteSurveyPoint::MapCoordinateMaximum)
        {
            ++mappedPointCount;
        }
    }

    // Labels are useful on a sparse plan, but become visual noise on a
    // dense survey. Keep all dots visible and suppress non-selected labels
    // once the Floor Plan contains more than twelve mapped Points.
    const bool showAllLabels =
        mappedPointCount <= 12U;

    // Draw ordinary Points first so the currently selected Point is always
    // visually on top when two markers overlap.
    for (uint8_t pass = 0;
         pass < 2;
         ++pass)
    {
        const bool selectedPass =
            pass == 1;

        for (uint8_t index = 0;
             index < count;
             ++index)
        {
            const StoredSiteSurveyPointIndex *point =
                StorageService::GetSavedSiteSurveyPointIndex(index);

            if (point == nullptr ||
                !point->available ||
                point->siteSurveyId != siteSurveyId ||
                point->floorPlanId != floorPlanViewerFloorPlanId ||
                point->mapX > StoredSiteSurveyPoint::MapCoordinateMaximum ||
                point->mapY > StoredSiteSurveyPoint::MapCoordinateMaximum)
            {
                continue;
            }

            const bool selected =
                point->pointId != 0 &&
                point->pointId == selectedSavedPointId;

            if (selected != selectedPass)
            {
                continue;
            }

            CreateFloorPlanPointMarker(
                *point,
                selected,
                showAllLabels || selected);
        }
    }

    Serial.printf(
        "MeasurementSetupScreen: Floor Plan %lu rendered %u Survey Point marker(s)\n",
        static_cast<unsigned long>(
            floorPlanViewerFloorPlanId),
        static_cast<unsigned int>(
            mappedPointCount));
}

void MeasurementSetupScreen::CreateFloorPlanPointMarker(
    const StoredSiteSurveyPointIndex &point,
    bool selected,
    bool showLabel)
{
    if (floorPlanCanvas == nullptr ||
        floorPlanRenderedWidth == 0 ||
        floorPlanRenderedHeight == 0)
    {
        return;
    }

    uint32_t renderedX = 0;
    uint32_t renderedY = 0;

    if (floorPlanRenderedWidth > 1)
    {
        renderedX =
            (static_cast<uint32_t>(point.mapX) *
                 (floorPlanRenderedWidth - 1U) +
             StoredSiteSurveyPoint::MapCoordinateMaximum / 2U) /
            StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    if (floorPlanRenderedHeight > 1)
    {
        renderedY =
            (static_cast<uint32_t>(point.mapY) *
                 (floorPlanRenderedHeight - 1U) +
             StoredSiteSurveyPoint::MapCoordinateMaximum / 2U) /
            StoredSiteSurveyPoint::MapCoordinateMaximum;
    }

    const int32_t pointX =
        static_cast<int32_t>(floorPlanRenderedOffsetX) +
        static_cast<int32_t>(renderedX);

    const int32_t pointY =
        static_cast<int32_t>(floorPlanRenderedOffsetY) +
        static_cast<int32_t>(renderedY);

    const lv_coord_t markerSize =
        selected ? 15 : 11;

    lv_obj_t *marker =
        lv_obj_create(floorPlanCanvas);

    lv_obj_remove_style_all(marker);

    lv_obj_set_size(
        marker,
        markerSize,
        markerSize);

    lv_obj_set_pos(
        marker,
        static_cast<lv_coord_t>(
            pointX - markerSize / 2),
        static_cast<lv_coord_t>(
            pointY - markerSize / 2));

    // Dual-tone markers remain visible over both light architectural
    // drawings and dark scanned plans. The selected Point uses a larger
    // red center so its identity is obvious without changing map geometry.
    lv_obj_set_style_radius(
        marker,
        LV_RADIUS_CIRCLE,
        0);

    lv_obj_set_style_bg_color(
        marker,
        selected
            ? lv_color_make(220, 32, 32)
            : lv_color_make(255, 255, 255),
        0);

    lv_obj_set_style_bg_opa(
        marker,
        LV_OPA_COVER,
        0);

    lv_obj_set_style_border_width(
        marker,
        2,
        0);

    lv_obj_set_style_border_color(
        marker,
        lv_color_make(0, 0, 0),
        0);

    lv_obj_set_style_border_opa(
        marker,
        LV_OPA_COVER,
        0);

    lv_obj_clear_flag(
        marker,
        LV_OBJ_FLAG_CLICKABLE);

    lv_obj_clear_flag(
        marker,
        LV_OBJ_FLAG_SCROLLABLE);

    if (!showLabel)
    {
        return;
    }

    lv_obj_t *label =
        lv_label_create(floorPlanCanvas);

    const lv_coord_t labelWidth =
        selected ? 112 : 38;

    lv_obj_set_width(
        label,
        labelWidth);

    lv_label_set_long_mode(
        label,
        LV_LABEL_LONG_DOT);

    char labelText[96] = {};

    if (selected &&
        point.name[0] != '\0')
    {
        std::snprintf(
            labelText,
            sizeof(labelText),
            "P%lu %s",
            static_cast<unsigned long>(point.pointId),
            point.name);
    }
    else
    {
        std::snprintf(
            labelText,
            sizeof(labelText),
            "P%lu",
            static_cast<unsigned long>(point.pointId));
    }

    lv_label_set_text(
        label,
        labelText);

    lv_obj_set_style_text_font(
        label,
        LV_FONT_DEFAULT,
        0);

    lv_obj_set_style_text_color(
        label,
        lv_color_make(255, 255, 255),
        0);

    lv_obj_set_style_bg_color(
        label,
        lv_color_make(0, 0, 0),
        0);

    lv_obj_set_style_bg_opa(
        label,
        LV_OPA_70,
        0);

    lv_obj_set_style_pad_left(
        label,
        2,
        0);

    lv_obj_set_style_pad_right(
        label,
        2,
        0);

    lv_obj_set_style_pad_top(
        label,
        1,
        0);

    lv_obj_set_style_pad_bottom(
        label,
        1,
        0);

    lv_obj_set_style_radius(
        label,
        2,
        0);

    lv_obj_clear_flag(
        label,
        LV_OBJ_FLAG_CLICKABLE);

    lv_obj_clear_flag(
        label,
        LV_OBJ_FLAG_SCROLLABLE);

    // Prefer the label to the right of the marker. Flip it left near the
    // viewport edge so the Point ID/name remains readable.
    int32_t labelX =
        pointX + markerSize / 2 + 3;

    if (labelX + labelWidth >
        static_cast<int32_t>(floorPlanViewportWidth))
    {
        labelX =
            pointX - markerSize / 2 - 3 - labelWidth;
    }

    if (labelX < 0)
    {
        labelX = 0;
    }

    int32_t labelY =
        pointY - 7;

    const int32_t estimatedLabelHeight = 14;

    if (labelY + estimatedLabelHeight >
        static_cast<int32_t>(floorPlanViewportHeight))
    {
        labelY =
            static_cast<int32_t>(floorPlanViewportHeight) -
            estimatedLabelHeight;
    }

    if (labelY < 0)
    {
        labelY = 0;
    }

    lv_obj_set_pos(
        label,
        static_cast<lv_coord_t>(labelX),
        static_cast<lv_coord_t>(labelY));
}

void MeasurementSetupScreen::ClearPendingMapPosition()
{
    pendingMapPositionValid = false;
    pendingMapPositionPersisted = false;
    pendingMapFloorPlanId = 0;
    pendingMapX = 0;
    pendingMapY = 0;
}

void MeasurementSetupScreen::RefreshFloorPlanButtonState()
{
    if (floorPlansButton == nullptr)
    {
        return;
    }

    const bool enabled =
        GetPointContextSurveyId() != 0 &&
        StorageService::IsAvailable() &&
        !StorageService::IsExternalReadOnlyAccessActive();

    if (enabled)
    {
        lv_obj_clear_state(
            floorPlansButton,
            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(
            floorPlansButton,
            LV_STATE_DISABLED);
    }
}

uint32_t MeasurementSetupScreen::GetPointContextSurveyId() const
{
    if (selectedSavedSurveyId != 0)
    {
        return selectedSavedSurveyId;
    }

    if (!SiteSurveyService::HasActiveSurvey() ||
        siteSurveyTextArea == nullptr)
    {
        return 0;
    }

    const SiteSurveyInfo &activeSurvey =
        SiteSurveyService::GetActiveSurvey();

    const char *displayedName =
        lv_textarea_get_text(siteSurveyTextArea);

    if (displayedName == nullptr ||
        std::strcmp(
            displayedName,
            activeSurvey.name) != 0)
    {
        return 0;
    }

    return activeSurvey.surveyId;
}

void MeasurementSetupScreen::RefreshSavedPointButtonState()
{
    if (selectSavedPointButton == nullptr)
    {
        return;
    }

    const uint32_t surveyId = GetPointContextSurveyId();
    bool hasSavedPoint = false;

    if (surveyId != 0)
    {
        const uint8_t count =
            StorageService::GetSavedSiteSurveyPointCount();

        for (uint8_t index = 0;
             index < count;
             ++index)
        {
            const StoredSiteSurveyPointIndex *point =
                StorageService::GetSavedSiteSurveyPointIndex(index);

            if (point != nullptr &&
                point->available &&
                point->siteSurveyId == surveyId)
            {
                hasSavedPoint = true;
                break;
            }
        }
    }

    if (hasSavedPoint)
    {
        lv_obj_clear_state(
            selectSavedPointButton,
            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(
            selectSavedPointButton,
            LV_STATE_DISABLED);
    }
}

void MeasurementSetupScreen::ClearSavedPointSelection(
    bool clearText)
{
    const bool hadSavedPoint =
        selectedSavedPointId != 0;

    selectedSavedPointId = 0;
    selectedSavedPointSiteSurveyId = 0;

    if (hadSavedPoint)
    {
        ClearPendingMapPosition();
    }

    if (clearText &&
        surveyPointTextArea != nullptr)
    {
        lv_textarea_set_text(
            surveyPointTextArea,
            "");
    }
}

void MeasurementSetupScreen::OpenPointSelector()
{
    if (pointSelectorRoot != nullptr)
    {
        return;
    }

    const uint32_t surveyId =
        GetPointContextSurveyId();

    if (surveyId == 0)
    {
        return;
    }

    const uint8_t pointCount =
        StorageService::GetSavedSiteSurveyPointCount();

    bool hasMatchingPoint = false;

    for (uint8_t index = 0;
         index < pointCount;
         ++index)
    {
        const StoredSiteSurveyPointIndex *point =
            StorageService::GetSavedSiteSurveyPointIndex(index);

        if (point != nullptr &&
            point->available &&
            point->siteSurveyId == surveyId)
        {
            hasMatchingPoint = true;
            break;
        }
    }

    if (!hasMatchingPoint)
    {
        return;
    }

    lv_obj_t *parent = lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    pointSelectorRoot = lv_obj_create(parent);

    lv_obj_set_pos(pointSelectorRoot, 0, 0);
    lv_obj_set_size(
        pointSelectorRoot,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        pointSelectorRoot,
        8,
        0);

    lv_obj_set_style_pad_row(
        pointSelectorRoot,
        6,
        0);

    lv_obj_set_flex_flow(
        pointSelectorRoot,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        pointSelectorRoot,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title =
        lv_label_create(pointSelectorRoot);

    lv_label_set_text(
        title,
        "SAVED SURVEY POINTS");

    lv_obj_t *pointList =
        lv_obj_create(pointSelectorRoot);

    lv_obj_set_width(pointList, lv_pct(100));
    lv_obj_set_height(pointList, 0);
    lv_obj_set_flex_grow(pointList, 1);
    lv_obj_add_flag(pointList, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(pointList, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(
        pointList,
        LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_all(pointList, 4, 0);
    lv_obj_set_style_pad_row(pointList, 4, 0);
    lv_obj_set_flex_flow(
        pointList,
        LV_FLEX_FLOW_COLUMN);

    for (uint8_t index = 0;
         index < pointCount;
         ++index)
    {
        const StoredSiteSurveyPointIndex *point =
            StorageService::GetSavedSiteSurveyPointIndex(index);

        if (point == nullptr ||
            !point->available ||
            point->siteSurveyId != surveyId)
        {
            continue;
        }

        lv_obj_t *button = lv_btn_create(pointList);

        lv_obj_set_width(button, lv_pct(100));
        lv_obj_set_height(button, 40);

        lv_obj_add_event_cb(
            button,
            MeasurementSetupScreen::HandleSavedPointButton,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(index)));

        lv_obj_t *label = lv_label_create(button);

        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);

        char text[72];

        std::snprintf(
            text,
            sizeof(text),
            "#%lu  %s",
            static_cast<unsigned long>(point->pointId),
            point->name);

        lv_label_set_text(label, text);
        lv_obj_center(label);
    }

    lv_obj_t *cancelButton =
        lv_btn_create(pointSelectorRoot);

    lv_obj_set_width(cancelButton, lv_pct(100));
    lv_obj_set_height(cancelButton, 34);

    lv_obj_t *cancelLabel =
        lv_label_create(cancelButton);

    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);

    lv_obj_add_event_cb(
        cancelButton,
        MeasurementSetupScreen::HandlePointSelectorCancel,
        LV_EVENT_CLICKED,
        nullptr);

    lv_obj_move_foreground(pointSelectorRoot);
}

void MeasurementSetupScreen::ClosePointSelector()
{
    if (pointSelectorRoot == nullptr)
    {
        return;
    }

    lv_obj_add_flag(
        pointSelectorRoot,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_del_async(pointSelectorRoot);
    pointSelectorRoot = nullptr;
}

void MeasurementSetupScreen::OpenTextEditor(
    lv_obj_t *target)
{
    if (target == nullptr ||
        editorRoot != nullptr)
    {
        return;
    }

    editorTarget = target;

    lv_obj_t *parent =
        lv_layer_top();

    editorRoot =
        lv_obj_create(parent);

    lv_obj_set_pos(
        editorRoot,
        0,
        0);

    lv_obj_set_size(
        editorRoot,
        lv_pct(100),
        lv_pct(100));

    lv_obj_set_style_pad_all(
        editorRoot,
        4,
        0);

    lv_obj_set_style_pad_row(
        editorRoot,
        4,
        0);

    lv_obj_set_flex_flow(
        editorRoot,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(
        editorRoot,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title =
        lv_label_create(editorRoot);

    if (target == siteSurveyTextArea)
    {
        lv_label_set_text(
            title,
            "EDIT SITE SURVEY");
    }
    else
    {
        lv_label_set_text(
            title,
            "EDIT SURVEY POINT");
    }

    editorTextArea =
        lv_textarea_create(editorRoot);

    lv_obj_set_width(
        editorTextArea,
        lv_pct(100));

    lv_obj_set_height(
        editorTextArea,
        34);

    lv_textarea_set_one_line(
        editorTextArea,
        true);

    if (target == siteSurveyTextArea)
    {
        lv_textarea_set_max_length(
            editorTextArea,
            SiteSurveyInfo::NameCapacity - 1);
    }
    else
    {
        lv_textarea_set_max_length(
            editorTextArea,
            31);
    }

    const char *currentText =
        lv_textarea_get_text(target);

    if (currentText != nullptr)
    {
        lv_textarea_set_text(
            editorTextArea,
            currentText);
    }

    editorKeyboard =
        lv_keyboard_create(editorRoot);

    lv_obj_set_width(
        editorKeyboard,
        lv_pct(100));

    lv_obj_set_flex_grow(
        editorKeyboard,
        1);

    lv_keyboard_set_mode(
        editorKeyboard,
        LV_KEYBOARD_MODE_TEXT_LOWER);

    lv_keyboard_set_textarea(
        editorKeyboard,
        editorTextArea);

    lv_obj_add_event_cb(
        editorKeyboard,
        MeasurementSetupScreen::HandleKeyboardEvent,
        LV_EVENT_READY,
        nullptr);

    lv_obj_add_event_cb(
        editorKeyboard,
        MeasurementSetupScreen::HandleKeyboardEvent,
        LV_EVENT_CANCEL,
        nullptr);

    lv_obj_move_foreground(
        editorRoot);
}

void MeasurementSetupScreen::CloseTextEditor(
    bool save)
{
    if (editorRoot == nullptr)
    {
        return;
    }

    const bool editedSiteSurvey =
        editorTarget == siteSurveyTextArea;

    if (save &&
        editorTarget != nullptr &&
        editorTextArea != nullptr)
    {
        const char *text =
            lv_textarea_get_text(
                editorTextArea);

        lv_textarea_set_text(
            editorTarget,
            text != nullptr
                ? text
                : "");
    }

    if (editedSiteSurvey)
    {
        RefreshSavedPointButtonState();
        RefreshFloorPlanButtonState();
    }

    lv_obj_add_flag(
        editorRoot,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_del_async(
        editorRoot);

    editorRoot = nullptr;
    editorTextArea = nullptr;
    editorKeyboard = nullptr;
    editorTarget = nullptr;

    // Measurement Setup is still open after the keyboard closes, so keep
    // global page-swipe navigation locked. Hide() is the single owner of
    // restoring gesture navigation when the modal workflow actually exits.
}

void MeasurementSetupScreen::HandleKeyboardEvent(
    lv_event_t *event)
{
    if (instance == nullptr ||
        event == nullptr)
    {
        return;
    }

    const lv_event_code_t code =
        lv_event_get_code(event);

    if (code == LV_EVENT_READY)
    {
        instance->CloseTextEditor(
            true);
    }
    else if (code == LV_EVENT_CANCEL)
    {
        instance->CloseTextEditor(
            false);
    }
}

void MeasurementSetupScreen::Hide()
{
    if (root == nullptr)
    {
        NavigationManager::SetGestureNavigationEnabled(true);
        return;
    }

    if (surveySelectorRoot != nullptr)
    {
        lv_obj_del_async(
            surveySelectorRoot);

        surveySelectorRoot = nullptr;
    }

    CloseFloorPlanViewer();

    if (floorPlanSelectorRoot != nullptr)
    {
        lv_obj_del_async(
            floorPlanSelectorRoot);

        floorPlanSelectorRoot = nullptr;
    }

    if (pointSelectorRoot != nullptr)
    {
        lv_obj_del_async(
            pointSelectorRoot);

        pointSelectorRoot = nullptr;
    }

    lv_obj_del_async(root);

    root = nullptr;
    selectSavedSurveyButton = nullptr;
    floorPlansButton = nullptr;
    selectSavedPointButton = nullptr;
    siteSurveyTextArea = nullptr;
    surveyPointTextArea = nullptr;
    closeSurveyButton = nullptr;
    cancelButton = nullptr;
    startButton = nullptr;
    selectedSavedSurveyId = 0;
    selectedSavedSurveyCreatedEpoch = 0;
    selectedSavedPointId = 0;
    selectedSavedPointSiteSurveyId = 0;
    ClearPendingMapPosition();
    instance = nullptr;

    editorRoot = nullptr;
    editorTextArea = nullptr;
    editorKeyboard = nullptr;
    editorTarget = nullptr;

    // Measurement Setup owns the global page-swipe lock for its entire
    // visible lifetime. Always release that lock when the modal closes,
    // regardless of whether it was exited by Start, Cancel, Close Survey,
    // or after returning from the Floor Plan workflow.
    NavigationManager::SetGestureNavigationEnabled(true);
}