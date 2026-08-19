#include <Arduino.h>

#include "MeasurementSetupScreen.h"

#include "../Services/Survey/SiteSurveyService.h"
#include "../Managers/SiteSurveyManager.h"
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
        6,
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
        38);

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
            36);

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
        36);

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
        36);

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

    uint32_t createdEpoch = 0;

    // Zero remains acceptable if wall-clock time is unavailable.
    TimeService::GetEpochTime(
        createdEpoch);

    if (!SiteSurveyManager::PrepareSurvey(
            surveyName,
            createdEpoch))
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

    instance->OpenTextEditor(
        target);
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

    lv_obj_del_async(root);

    root = nullptr;
    siteSurveyTextArea = nullptr;
    surveyPointTextArea = nullptr;
    closeSurveyButton = nullptr;
    cancelButton = nullptr;
    startButton = nullptr;
    instance = nullptr;

    editorRoot = nullptr;
    editorTextArea = nullptr;
    editorKeyboard = nullptr;
    editorTarget = nullptr;
}