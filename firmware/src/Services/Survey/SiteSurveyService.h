#pragma once

#include "SiteSurveyTypes.h"

class SiteSurveyService
{
public:
    static void Begin();

    static bool StartSurvey(
        uint32_t surveyId,
        const char *name,
        uint32_t createdEpoch);

    static void CloseSurvey();

    static bool HasActiveSurvey();

    static const SiteSurveyInfo &
        GetActiveSurvey();

private:
    static SiteSurveyInfo activeSurvey;
};