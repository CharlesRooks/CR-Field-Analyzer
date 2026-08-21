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

    static bool CloseSurvey();
};