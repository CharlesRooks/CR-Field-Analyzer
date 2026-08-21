#pragma once

#include <stdint.h>
#include <lvgl.h>

enum class MeasurementSetupAction : uint8_t
{
    SingleScan = 0,
    MeasurementSession
};

class MeasurementSetupScreen
{
public:
    void Show(
        MeasurementSetupAction action);

    void Hide();

    bool IsVisible() const
    {
        return root != nullptr;
    }

private:
    MeasurementSetupAction pendingAction =
        MeasurementSetupAction::SingleScan;

    lv_obj_t *root = nullptr;

    lv_obj_t *siteSurveyTextArea = nullptr;
    lv_obj_t *surveyPointTextArea = nullptr;

    lv_obj_t *selectSavedSurveyButton = nullptr;
    lv_obj_t *closeSurveyButton = nullptr;
    lv_obj_t *cancelButton = nullptr;
    lv_obj_t *startButton = nullptr;

    lv_obj_t *surveySelectorRoot = nullptr;

    uint32_t selectedSavedSurveyId = 0;
    uint32_t selectedSavedSurveyCreatedEpoch = 0;

    lv_obj_t *editorRoot = nullptr;
    lv_obj_t *editorTextArea = nullptr;
    lv_obj_t *editorKeyboard = nullptr;

    lv_obj_t *editorTarget = nullptr;

    static void HandleTextAreaFocus(
    lv_event_t *event);

    static void HandleKeyboardEvent(
        lv_event_t *event);

    static void HandleSelectSavedSurveyButton(
        lv_event_t *event);

    static void HandleSavedSurveyButton(
        lv_event_t *event);

    static void HandleSurveySelectorCancel(
        lv_event_t *event);

    static void HandleCloseSurveyButton(
        lv_event_t *event);

    static void HandleCancelButton(
        lv_event_t *event);

    static void HandleStartButton(
        lv_event_t *event);

    void OpenSurveySelector();

    void CloseSurveySelector();

    void OpenTextEditor(
        lv_obj_t *target);

    void CloseTextEditor(
        bool save);
};