#pragma once

#include <stdint.h>

class SiteSurveyManager
{
public:
    static void Begin();

    static bool StartSurvey(
        const char *name,
        uint32_t createdEpoch);

    static bool ResumeSurvey(
        uint32_t surveyId,
        const char *name,
        uint32_t createdEpoch);

    static bool PrepareSavedSurvey(
        uint32_t surveyId,
        const char *name,
        uint32_t createdEpoch);

    static bool PrepareSurvey(
        const char *name,
        uint32_t createdEpoch);

    static bool PrepareNewSurveyPoint(
        const char *name,
        uint32_t createdEpoch);

    static bool PrepareSavedSurveyPoint(
        uint32_t pointId,
        uint32_t siteSurveyId,
        const char *name);

    static bool RegisterFloorPlanImport(
        uint32_t siteSurveyId,
        const char *importPath,
        uint32_t createdEpoch,
        uint32_t &floorPlanId);

    static bool CloseSurvey();
};