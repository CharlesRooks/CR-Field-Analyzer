#include <Arduino.h>

#include "MeasurementSetupScreen.h"

#include "../Services/Survey/SiteSurveyService.h"
#include "../Managers/SiteSurveyManager.h"
#include "../Services/Storage/StorageService.h"
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

    if (StorageService::GetSavedSiteSurveyCount() > 0)
    {
        selectSavedSurveyButton =
            lv_btn_create(root);

        lv_obj_set_width(
            selectSavedSurveyButton,
            lv_pct(100));

        lv_obj_set_height(
            selectSavedSurveyButton,
            32);

        lv_obj_t *selectSavedSurveyLabel =
            lv_label_create(
                selectSavedSurveyButton);

        lv_label_set_text(
            selectSavedSurveyLabel,
            "Select Saved Survey");

        lv_obj_center(
            selectSavedSurveyLabel);

        lv_obj_add_event_cb(
            selectSavedSurveyButton,
            MeasurementSetupScreen::
                HandleSelectSavedSurveyButton,
            LV_EVENT_CLICKED,
            nullptr);
    }

    // ------------------------------------------------------------
    // Survey Point
    // ------------------------------------------------------------

    lv_obj_t *pointLabel =
        lv_label_create(root);

    lv_label_set_text(
        pointLabel,
        "Survey Point");

    surveyPointTextArea =
        lv_textarea_create(root);

    lv_obj_add_event_cb(
        surveyPointTextArea,
        MeasurementSetupScreen::HandleTextAreaFocus,
        LV_EVENT_FOCUSED,
        nullptr);

    lv_obj_set_width(
        surveyPointTextArea,
        lv_pct(100));

    lv_obj_set_height(
        surveyPointTextArea,
        34);

    lv_textarea_set_one_line(
        surveyPointTextArea,
        true);

    lv_textarea_set_max_length(
        surveyPointTextArea,
        31);

    lv_textarea_set_placeholder_text(
        surveyPointTextArea,
        "e.g. Conference Room");

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

    instance->selectedSavedSurveyId =
        survey->surveyId;

    instance->selectedSavedSurveyCreatedEpoch =
        survey->createdEpoch;

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

    if (!WiFiService::SetMeasurementSurveyPoint(
            surveyPoint))
    {
        Serial.println(
            "MeasurementSetupScreen: "
            "Survey Point could not be assigned");

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
        // explicit selection of a stored survey.
        instance->selectedSavedSurveyId = 0;
        instance->selectedSavedSurveyCreatedEpoch = 0;
    }

    instance->OpenTextEditor(
        target);
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

    lv_obj_del_async(root);

    root = nullptr;
    selectSavedSurveyButton = nullptr;
    siteSurveyTextArea = nullptr;
    surveyPointTextArea = nullptr;
    closeSurveyButton = nullptr;
    cancelButton = nullptr;
    startButton = nullptr;
    selectedSavedSurveyId = 0;
    selectedSavedSurveyCreatedEpoch = 0;
    instance = nullptr;

    editorRoot = nullptr;
    editorTextArea = nullptr;
    editorKeyboard = nullptr;
    editorTarget = nullptr;
}