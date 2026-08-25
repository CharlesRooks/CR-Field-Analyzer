#include <Arduino.h>
#include <cstring>
#include <esp_heap_caps.h>

#include "MeasurementSetupScreen.h"

#include "../Services/Survey/SiteSurveyService.h"
#include "../Managers/SiteSurveyManager.h"
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
        return;
    }

    selectedSavedSurveyId = 0;
    selectedSavedSurveyCreatedEpoch = 0;
    selectedSavedPointId = 0;
    selectedSavedPointSiteSurveyId = 0;

    lv_obj_t *parent =
        lv_layer_top();

    if (parent == nullptr)
    {
        return;
    }

    root =
        lv_obj_create(parent);

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

    instance->CloseFloorPlanViewer();
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
        "Close");

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
        54);

    lv_obj_set_height(
        backButton,
        26);

    lv_obj_t *backLabel =
        lv_label_create(backButton);

    lv_label_set_text(
        backLabel,
        "Back");

    lv_obj_center(backLabel);

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

    lv_obj_t *fitLabel =
        lv_label_create(header);

    lv_label_set_text(
        fitLabel,
        "FIT");

    // ------------------------------------------------------------
    // Full-width Floor Plan viewport
    // ------------------------------------------------------------

    lv_obj_t *viewport =
        lv_obj_create(floorPlanViewerRoot);

    lv_obj_remove_style_all(viewport);

    lv_obj_set_width(
        viewport,
        lv_pct(100));

    lv_obj_set_height(
        viewport,
        0);

    lv_obj_set_flex_grow(
        viewport,
        1);

    lv_obj_clear_flag(
        viewport,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_update_layout(
        floorPlanViewerRoot);

    const lv_coord_t viewportWidth =
        lv_obj_get_width(viewport);

    const lv_coord_t viewportHeight =
        lv_obj_get_height(viewport);

    if (viewportWidth <= 0 ||
        viewportHeight <= 0)
    {
        lv_obj_t *errorLabel =
            lv_label_create(viewport);

        lv_label_set_text(
            errorLabel,
            "Floor Plan viewport unavailable");

        lv_obj_center(errorLabel);
        lv_obj_move_foreground(
            floorPlanViewerRoot);
        return;
    }

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
            lv_label_create(viewport);

        lv_label_set_text(
            errorLabel,
            "Not enough memory to open Floor Plan");

        lv_obj_center(errorLabel);
        lv_obj_move_foreground(
            floorPlanViewerRoot);
        return;
    }

    floorPlanCanvas =
        lv_canvas_create(viewport);

    lv_canvas_set_buffer(
        floorPlanCanvas,
        floorPlanCanvasBuffer,
        static_cast<lv_coord_t>(viewportWidth),
        static_cast<lv_coord_t>(viewportHeight),
        LV_IMG_CF_TRUE_COLOR);

    lv_obj_center(
        floorPlanCanvas);

    FloorPlanRenderInfo renderInfo{};

    if (!FloorPlanImageRenderer::RenderFit(
            floorPlan->imagePath,
            floorPlanCanvasBuffer,
            static_cast<uint16_t>(viewportWidth),
            static_cast<uint16_t>(viewportHeight),
            renderInfo))
    {
        lv_obj_t *errorLabel =
            lv_label_create(viewport);

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
        lv_obj_invalidate(
            floorPlanCanvas);

        Serial.printf(
            "MeasurementSetupScreen: Viewing Floor Plan %lu: %s\n",
            static_cast<unsigned long>(
                floorPlan->floorPlanId),
            floorPlan->imagePath);
    }

    lv_obj_move_foreground(
        floorPlanViewerRoot);
}

void MeasurementSetupScreen::CloseFloorPlanViewer()
{
    if (floorPlanViewerRoot != nullptr)
    {
        // The canvas uses an externally allocated PSRAM buffer, so delete
        // the LVGL object synchronously before releasing the buffer.
        lv_obj_del(
            floorPlanViewerRoot);

        floorPlanViewerRoot = nullptr;
        floorPlanCanvas = nullptr;
    }

    if (floorPlanCanvasBuffer != nullptr)
    {
        heap_caps_free(
            floorPlanCanvasBuffer);

        floorPlanCanvasBuffer = nullptr;
    }
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
    selectedSavedPointId = 0;
    selectedSavedPointSiteSurveyId = 0;

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
    instance = nullptr;

    editorRoot = nullptr;
    editorTextArea = nullptr;
    editorKeyboard = nullptr;
    editorTarget = nullptr;
}