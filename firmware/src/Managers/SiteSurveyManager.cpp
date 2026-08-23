#include <cstring>

#include "SiteSurveyManager.h"

#include "../Services/Storage/StorageService.h"
#include "../Services/Survey/SiteSurveyService.h"
#include "../Services/WiFi/WiFiService.h"

void SiteSurveyManager::Begin()
{
    // Start from a known clear Wi-Fi measurement context.
    WiFiService::SetMeasurementSiteSurvey(
        0,
        nullptr);

    StoredActiveSiteSurvey storedSurvey{};

    if (!StorageService::LoadActiveSiteSurvey(
            storedSurvey))
    {
        Serial.println(
            "SiteSurveyManager: No active Site Survey "
            "to restore");
        return;
    }

    if (!SiteSurveyService::StartSurvey(
            storedSurvey.surveyId,
            storedSurvey.name,
            storedSurvey.createdEpoch))
    {
        Serial.printf(
            "SiteSurveyManager: Stored active survey %lu "
            "could not be restored\n",
            static_cast<unsigned long>(
                storedSurvey.surveyId));

        return;
    }

    if (!WiFiService::SetMeasurementSiteSurvey(
            storedSurvey.surveyId,
            storedSurvey.name))
    {
        SiteSurveyService::CloseSurvey();

        Serial.printf(
            "SiteSurveyManager: Restored survey %lu "
            "could not be synchronized with Wi-Fi\n",
            static_cast<unsigned long>(
                storedSurvey.surveyId));

        return;
    }

    Serial.printf(
        "SiteSurveyManager: Restored active survey %lu: %s\n",
        static_cast<unsigned long>(
            storedSurvey.surveyId),
        storedSurvey.name);
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

    if (!StorageService::SaveActiveSiteSurvey(
            survey.surveyId,
            survey.name,
            survey.createdEpoch))
    {
        WiFiService::SetMeasurementSiteSurvey(
            0,
            nullptr);

        SiteSurveyService::CloseSurvey();

        Serial.printf(
            "SiteSurveyManager: Survey %lu could not be "
            "persisted as the active survey\n",
            static_cast<unsigned long>(
                survey.surveyId));

        return false;
    }

    Serial.printf(
        "SiteSurveyManager: Survey %lu active: %s\n",
        static_cast<unsigned long>(
            survey.surveyId),
        survey.name);

    return true;
}

bool SiteSurveyManager::ResumeSurvey(
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

    // Resuming is only valid when no Site Survey
    // is currently active.
    if (SiteSurveyService::HasActiveSurvey())
    {
        return false;
    }

    // Do not change survey context while a Wi-Fi
    // measurement is already in progress.
    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    // Unlike StartSurvey(), no permanent survey
    // record is created here. The supplied ID
    // already belongs to an existing survey.
    if (!SiteSurveyService::StartSurvey(
            surveyId,
            name,
            createdEpoch))
    {
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
            "SiteSurveyManager: Survey %lu could not "
            "be resumed in Wi-Fi context\n",
            static_cast<unsigned long>(
                surveyId));

        return false;
    }

    if (!StorageService::SaveActiveSiteSurvey(
            survey.surveyId,
            survey.name,
            survey.createdEpoch))
    {
        WiFiService::SetMeasurementSiteSurvey(
            0,
            nullptr);

        SiteSurveyService::CloseSurvey();

        Serial.printf(
            "SiteSurveyManager: Survey %lu could not "
            "be persisted as the active survey\n",
            static_cast<unsigned long>(
                surveyId));

        return false;
    }

    Serial.printf(
        "SiteSurveyManager: Survey %lu resumed: %s\n",
        static_cast<unsigned long>(
            survey.surveyId),
        survey.name);

    return true;
}

bool SiteSurveyManager::PrepareSavedSurvey(
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

    // Nothing is currently active, so the stored
    // survey can be resumed directly.
    if (!SiteSurveyService::HasActiveSurvey())
    {
        return ResumeSurvey(
            surveyId,
            name,
            createdEpoch);
    }

    const SiteSurveyInfo current =
        SiteSurveyService::GetActiveSurvey();

    // Selecting the survey that is already active
    // simply continues the existing context.
    if (current.surveyId == surveyId)
    {
        if (std::strcmp(
                current.name,
                name) != 0)
        {
            Serial.printf(
                "SiteSurveyManager: Survey %lu name "
                "does not match active survey\n",
                static_cast<unsigned long>(
                    surveyId));

            return false;
        }

        return WiFiService::
            SetMeasurementSiteSurvey(
                current.surveyId,
                current.name);
    }

    // Switching to a different stored survey requires
    // closing the current active context first.
    if (!CloseSurvey())
    {
        return false;
    }

    if (ResumeSurvey(
            surveyId,
            name,
            createdEpoch))
    {
        return true;
    }

    // The selected survey could not be resumed.
    // Restore the previous active survey so a failed
    // switch does not unnecessarily lose its context.
    if (!ResumeSurvey(
            current.surveyId,
            current.name,
            current.createdEpoch))
    {
        Serial.printf(
            "SiteSurveyManager: Previous survey %lu "
            "could not be restored after failed resume\n",
            static_cast<unsigned long>(
                current.surveyId));
    }
    else
    {
        Serial.printf(
            "SiteSurveyManager: Previous survey %lu "
            "restored after failed resume\n",
            static_cast<unsigned long>(
                current.surveyId));
    }

    return false;
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
    const bool serviceRestored =
        SiteSurveyService::StartSurvey(
            current.surveyId,
            current.name,
            current.createdEpoch);

    bool wifiRestored = false;
    bool storageRestored = false;

    if (serviceRestored)
    {
        wifiRestored =
            WiFiService::SetMeasurementSiteSurvey(
                current.surveyId,
                current.name);
    }

    if (serviceRestored &&
        wifiRestored)
    {
        storageRestored =
            StorageService::SaveActiveSiteSurvey(
                current.surveyId,
                current.name,
                current.createdEpoch);
    }

    if (!serviceRestored ||
        !wifiRestored ||
        !storageRestored)
    {
        Serial.println(
            "SiteSurveyManager: Previous survey "
            "could not be fully restored");
    }
    else
    {
        Serial.printf(
            "SiteSurveyManager: Previous survey %lu "
            "restored after failed survey switch\n",
            static_cast<unsigned long>(
                current.surveyId));
    }

    return false;
}

bool SiteSurveyManager::PrepareNewSurveyPoint(
    const char *name,
    uint32_t createdEpoch)
{
    if (name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    if (!SiteSurveyService::HasActiveSurvey())
    {
        Serial.println(
            "SiteSurveyManager: Survey Point creation "
            "requires an active Site Survey");

        return false;
    }

    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    const SiteSurveyInfo &survey =
        SiteSurveyService::GetActiveSurvey();

    uint32_t pointId = 0;

    if (!StorageService::
            CreateSiteSurveyPointRecord(
                survey.surveyId,
                name,
                createdEpoch,
                pointId))
    {
        Serial.println(
            "SiteSurveyManager: Survey Point "
            "could not be created");

        return false;
    }

    if (!WiFiService::
            SetMeasurementSurveyPoint(
                pointId,
                name))
    {
        Serial.printf(
            "SiteSurveyManager: Survey Point %lu "
            "was stored but could not be assigned "
            "to the measurement\n",
            static_cast<unsigned long>(pointId));

        return false;
    }

    Serial.printf(
        "SiteSurveyManager: Survey Point %lu "
        "prepared for Site Survey %lu: %s\n",
        static_cast<unsigned long>(pointId),
        static_cast<unsigned long>(survey.surveyId),
        name);

    return true;
}

bool SiteSurveyManager::PrepareSavedSurveyPoint(
    uint32_t pointId,
    uint32_t siteSurveyId,
    const char *name)
{
    if (pointId == 0 ||
        siteSurveyId == 0 ||
        name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    if (!SiteSurveyService::HasActiveSurvey())
    {
        return false;
    }

    if (WiFiService::GetState() ==
            WiFiScanState::Scanning ||
        WiFiService::
            IsAutomaticMeasurementSessionActive())
    {
        return false;
    }

    const SiteSurveyInfo &survey =
        SiteSurveyService::GetActiveSurvey();

    if (survey.surveyId != siteSurveyId)
    {
        Serial.printf(
            "SiteSurveyManager: Survey Point %lu "
            "belongs to Site Survey %lu, not active "
            "Site Survey %lu\n",
            static_cast<unsigned long>(pointId),
            static_cast<unsigned long>(siteSurveyId),
            static_cast<unsigned long>(survey.surveyId));

        return false;
    }

    const uint8_t pointCount =
        StorageService::GetSavedSiteSurveyPointCount();

    const StoredSiteSurveyPointIndex *matchedPoint = nullptr;

    for (uint8_t index = 0;
         index < pointCount;
         ++index)
    {
        const StoredSiteSurveyPointIndex *point =
            StorageService::GetSavedSiteSurveyPointIndex(index);

        if (point != nullptr &&
            point->available &&
            point->pointId == pointId)
        {
            matchedPoint = point;
            break;
        }
    }

    if (matchedPoint == nullptr ||
        matchedPoint->siteSurveyId != siteSurveyId ||
        std::strcmp(
            matchedPoint->name,
            name) != 0)
    {
        Serial.printf(
            "SiteSurveyManager: Saved Survey Point %lu "
            "identity could not be validated\n",
            static_cast<unsigned long>(pointId));

        return false;
    }

    if (!WiFiService::SetMeasurementSurveyPoint(
            matchedPoint->pointId,
            matchedPoint->name))
    {
        return false;
    }

    Serial.printf(
        "SiteSurveyManager: Saved Survey Point %lu "
        "prepared for Site Survey %lu: %s\n",
        static_cast<unsigned long>(matchedPoint->pointId),
        static_cast<unsigned long>(matchedPoint->siteSurveyId),
        matchedPoint->name);

    return true;
}

bool SiteSurveyManager::CloseSurvey()
{
    SiteSurveyInfo current{};

    const bool hadActiveSurvey =
        SiteSurveyService::HasActiveSurvey();

    if (hadActiveSurvey)
    {
        current =
            SiteSurveyService::GetActiveSurvey();
    }

    // Clear the Wi-Fi assignment first. If a measurement is active,
    // WiFiService will reject the change and the survey remains open.
    if (!WiFiService::SetMeasurementSiteSurvey(
            0,
            nullptr))
    {
        return false;
    }

    // The persistent active-survey marker must also be removed.
    // Otherwise the survey would incorrectly return after reboot.
    if (!StorageService::ClearActiveSiteSurvey())
    {
        // Restore the Wi-Fi assignment if persistence could not
        // be cleared so RAM and storage remain consistent.
        if (hadActiveSurvey)
        {
            WiFiService::SetMeasurementSiteSurvey(
                current.surveyId,
                current.name);
        }

        Serial.println(
            "SiteSurveyManager: Active survey could not "
            "be closed because persistence could not "
            "be cleared");

        return false;
    }

    SiteSurveyService::CloseSurvey();

    if (hadActiveSurvey)
    {
        Serial.printf(
            "SiteSurveyManager: Survey %lu closed\n",
            static_cast<unsigned long>(
                current.surveyId));
    }

    return true;
}