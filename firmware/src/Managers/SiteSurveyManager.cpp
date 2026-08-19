#include <cstring>

#include "SiteSurveyManager.h"

#include "../Services/Storage/StorageService.h"
#include "../Services/Survey/SiteSurveyService.h"
#include "../Services/WiFi/WiFiService.h"

void SiteSurveyManager::Begin()
{
    // SentinelOS starts with no active Site Survey.
    WiFiService::SetMeasurementSiteSurvey(
        0,
        nullptr);
}

bool SiteSurveyManager::StartSurvey(
    const char *name,
    uint32_t createdEpoch)
{
    if (name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    // Only one Site Survey can be active at a time.
    if (SiteSurveyService::HasActiveSurvey())
    {
        return false;
    }

    // Never create a new survey while Wi-Fi measurement activity
    // is already in progress. This also prevents creating a survey
    // file that cannot immediately become the active survey.
    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    uint32_t surveyId = 0;

    if (!StorageService::CreateSiteSurveyRecord(
            name,
            createdEpoch,
            surveyId))
    {
        return false;
    }

    if (!SiteSurveyService::StartSurvey(
            surveyId,
            name,
            createdEpoch))
    {
        Serial.printf(
            "SiteSurveyManager: Survey %lu was stored "
            "but could not be activated\n",
            static_cast<unsigned long>(surveyId));

        return false;
    }

    const SiteSurveyInfo &survey =
        SiteSurveyService::GetActiveSurvey();

    if (!WiFiService::SetMeasurementSiteSurvey(
            survey.surveyId,
            survey.name))
    {
        SiteSurveyService::CloseSurvey();

        Serial.printf(
            "SiteSurveyManager: Survey %lu activation "
            "could not be synchronized with Wi-Fi\n",
            static_cast<unsigned long>(surveyId));

        return false;
    }

    Serial.printf(
        "SiteSurveyManager: Survey %lu active: %s\n",
        static_cast<unsigned long>(
            survey.surveyId),
        survey.name);

    return true;
}

bool SiteSurveyManager::PrepareSurvey(
    const char *name,
    uint32_t createdEpoch)
{
    if (name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    // No survey exists yet: create one normally.
    if (!SiteSurveyService::HasActiveSurvey())
    {
        return StartSurvey(
            name,
            createdEpoch);
    }

    const SiteSurveyInfo current =
        SiteSurveyService::GetActiveSurvey();

    // Same survey name means continue the existing survey.
    if (std::strcmp(
            current.name,
            name) == 0)
    {
        return WiFiService::
            SetMeasurementSiteSurvey(
                current.surveyId,
                current.name);
    }

    // A different name means the operator is moving to a
    // different Site Survey. Close the current survey first.
    if (!CloseSurvey())
    {
        return false;
    }

    if (StartSurvey(
            name,
            createdEpoch))
    {
        return true;
    }

    // If the new survey could not be created, restore the
    // previous active survey so the operator does not lose
    // the current survey context because of a storage failure.
    const bool restored =
        SiteSurveyService::StartSurvey(
            current.surveyId,
            current.name,
            current.createdEpoch) &&
        WiFiService::SetMeasurementSiteSurvey(
            current.surveyId,
            current.name);

    if (!restored)
    {
        Serial.println(
            "SiteSurveyManager: Previous survey "
            "could not be restored");
    }

    return false;
}

bool SiteSurveyManager::CloseSurvey()
{
    // Clear the Wi-Fi assignment first. If a measurement is active,
    // WiFiService will reject the change and the survey remains open.
    if (!WiFiService::SetMeasurementSiteSurvey(
            0,
            nullptr))
    {
        return false;
    }

    SiteSurveyService::CloseSurvey();

    return true;
}