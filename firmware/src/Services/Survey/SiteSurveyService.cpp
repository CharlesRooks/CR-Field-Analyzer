#include "SiteSurveyService.h"

#include <cstring>

SiteSurveyInfo
    SiteSurveyService::activeSurvey{};

void SiteSurveyService::Begin()
{
    activeSurvey = SiteSurveyInfo{};
}

bool SiteSurveyService::StartSurvey(
    uint32_t surveyId,
    const char *name,
    uint32_t createdEpoch)
{
    if (surveyId == 0 ||
        name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    SiteSurveyInfo survey{};

    survey.active = true;
    survey.surveyId = surveyId;
    survey.createdEpoch = createdEpoch;

    std::strncpy(
        survey.name,
        name,
        SiteSurveyInfo::NameCapacity - 1);

    survey.name[
        SiteSurveyInfo::NameCapacity - 1] = '\0';

    activeSurvey = survey;

    return true;
}

void SiteSurveyService::CloseSurvey()
{
    activeSurvey = SiteSurveyInfo{};
}

bool SiteSurveyService::HasActiveSurvey()
{
    return activeSurvey.active;
}

const SiteSurveyInfo &
SiteSurveyService::GetActiveSurvey()
{
    return activeSurvey;
}