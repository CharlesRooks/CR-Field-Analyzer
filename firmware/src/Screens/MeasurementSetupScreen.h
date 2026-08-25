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
    lv_obj_t *floorPlansButton = nullptr;
    lv_obj_t *selectSavedPointButton = nullptr;
    lv_obj_t *closeSurveyButton = nullptr;
    lv_obj_t *cancelButton = nullptr;
    lv_obj_t *startButton = nullptr;

    lv_obj_t *surveySelectorRoot = nullptr;
    lv_obj_t *floorPlanSelectorRoot = nullptr;
    lv_obj_t *floorPlanViewerRoot = nullptr;
    lv_obj_t *floorPlanCanvas = nullptr;
    lv_color_t *floorPlanCanvasBuffer = nullptr;
    lv_obj_t *pointSelectorRoot = nullptr;

    uint32_t selectedSavedSurveyId = 0;
    uint32_t selectedSavedSurveyCreatedEpoch = 0;

    uint32_t selectedSavedPointId = 0;
    uint32_t selectedSavedPointSiteSurveyId = 0;

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

    static void HandleFloorPlansButton(
        lv_event_t *event);

    static void HandleFloorPlanImportButton(
        lv_event_t *event);

    static void HandleRegisteredFloorPlanButton(
        lv_event_t *event);

    static void HandleFloorPlanViewerClose(
        lv_event_t *event);

    static void HandleFloorPlanSelectorClose(
        lv_event_t *event);

    static void HandleSelectSavedPointButton(
        lv_event_t *event);

    static void HandleSavedPointButton(
        lv_event_t *event);

    static void HandlePointSelectorCancel(
        lv_event_t *event);

    static void HandleCloseSurveyButton(
        lv_event_t *event);

    static void HandleCancelButton(
        lv_event_t *event);

    static void HandleStartButton(
        lv_event_t *event);

    void OpenSurveySelector();

    void CloseSurveySelector();

    void OpenFloorPlanSelector();

    void CloseFloorPlanSelector();

    void OpenFloorPlanViewer(
        uint8_t savedFloorPlanIndex);

    void CloseFloorPlanViewer();

    void RefreshFloorPlanButtonState();

    void OpenPointSelector();

    void ClosePointSelector();

    uint32_t GetPointContextSurveyId() const;

    void RefreshSavedPointButtonState();

    void ClearSavedPointSelection(
        bool clearText);

    void OpenTextEditor(
        lv_obj_t *target);

    void CloseTextEditor(
        bool save);
};