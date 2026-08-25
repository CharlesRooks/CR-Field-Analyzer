#pragma once

#include "StorageTypes.h"

#include <FS.h>
#include <stddef.h>
#include <stdint.h>

enum class StorageValidationResult : uint8_t
{
    NotRun = 0,
    Passed,
    CardUnavailable,
    DirectoryCreateFailed,
    StaleFileCleanupFailed,
    FileCreateFailed,
    FileWriteFailed,
    FileReadOpenFailed,
    FileReadFailed,
    ContentMismatch,
    TestFileCleanupFailed
};

class StorageService
{
public:
    // Milestone 10.18 expands retained history while keeping only a
    // lightweight SD-backed index in RAM. Full session details are
    // loaded and CRC-checked only when the user opens a record.
    static constexpr uint8_t MaxSavedSessions = 100;

    static void Begin();

    // Retains the Milestone 10.15A diagnostic without running a
    // destructive write/delete test during every normal startup.
    static bool RunValidation();

    static bool IsAvailable();

    // While USB Transfer Mode has the SD card exposed read/write to a host,
    // SentinelOS must not access or modify the filesystem. The legacy API
    // name is retained for compatibility with existing storage guards.
    static void SetExternalReadOnlyAccessActive(bool active);
    static bool IsExternalReadOnlyAccessActive();

    static bool ValidationPassed();

    static StorageValidationResult GetValidationResult();
    static const char *GetValidationResultText();
    static const char *GetCardTypeText();

    static uint64_t GetCardCapacityBytes();
    static uint64_t GetFilesystemTotalBytes();
    static uint64_t GetFilesystemUsedBytes();
    static uint64_t GetFilesystemFreeBytes();

    static uint32_t GetNextSiteSurveyId();

    static constexpr uint8_t MaxSavedSiteSurveys = 64;

    static uint8_t GetSavedSiteSurveyCount();

    static const StoredSiteSurveyIndex *
        GetSavedSiteSurveyIndex(uint8_t index);

    static constexpr uint8_t MaxSavedFloorPlans = 64;

    static uint8_t GetSavedFloorPlanCount();

    static const StoredFloorPlanIndex *
        GetSavedFloorPlanIndex(uint8_t index);

    static uint32_t GetNextFloorPlanId();

    static bool CreateFloorPlanRecord(
        uint32_t siteSurveyId,
        const char *name,
        const char *imagePath,
        uint16_t sourceWidth,
        uint16_t sourceHeight,
        uint32_t createdEpoch,
        uint32_t &floorPlanId);

    static constexpr uint8_t
        MaxFloorPlanImportImages = 32;

    static uint8_t RefreshFloorPlanImportCatalog();

    static uint8_t GetFloorPlanImportCount();

    static const FloorPlanImportImage *
        GetFloorPlanImportImage(uint8_t index);

    static bool RegisterImportedFloorPlan(
        uint32_t siteSurveyId,
        const char *importPath,
        uint32_t createdEpoch,
        uint32_t &floorPlanId);

    static constexpr uint8_t
        MaxSavedSiteSurveyPoints = 128;

    static uint8_t
        GetSavedSiteSurveyPointCount();

    static const StoredSiteSurveyPointIndex *
        GetSavedSiteSurveyPointIndex(
            uint8_t index);

    static uint32_t
        GetNextSiteSurveyPointId();

    static bool CreateSiteSurveyPointRecord(
        uint32_t siteSurveyId,
        const char *name,
        uint32_t createdEpoch,
        uint32_t &pointId);

    // Assigns or clears a persistent map position. floorPlanId == 0
    // clears the mapping and requires mapX/mapY to both be zero.
    static bool SetSiteSurveyPointMapPosition(
        uint32_t pointId,
        uint32_t floorPlanId,
        uint16_t mapX,
        uint16_t mapY);

    // Physical AP inventory is distinct from Survey Points: an AP
    // describes infrastructure location, while a Survey Point describes
    // where the technician stood to capture RF measurements.
    static constexpr uint8_t MaxSavedPhysicalAccessPoints = 64;

    static uint8_t GetSavedPhysicalAccessPointCount();

    static const StoredPhysicalAccessPointIndex *
        GetSavedPhysicalAccessPointIndex(uint8_t index);

    static uint32_t GetNextPhysicalAccessPointId();

    static bool CreatePhysicalAccessPointRecord(
        uint32_t siteSurveyId,
        const char *name,
        uint32_t createdEpoch,
        uint32_t &accessPointId);

    static bool LoadPhysicalAccessPoint(
        uint32_t accessPointId,
        StoredPhysicalAccessPoint &accessPoint);

    // floorPlanId == 0 clears the AP map position and requires both
    // normalized coordinates to be zero.
    static bool SetPhysicalAccessPointMapPosition(
        uint32_t accessPointId,
        uint32_t floorPlanId,
        uint16_t mapX,
        uint16_t mapY);

    // Replaces the complete BSSID association set for one physical AP.
    // Passing bssidCount == 0 clears all radio/BSSID associations.
    static bool SetPhysicalAccessPointBssids(
        uint32_t accessPointId,
        const StoredPhysicalAccessPointBssid *bssids,
        uint8_t bssidCount);

    static bool SaveMeasurementSummary(
        const WiFiMeasurementSummary &summary,
        uint32_t completedAtMs,
        uint32_t capturedEpoch,
        const char *capturedLocal);

    static bool CreateSiteSurveyRecord(
        const char *name,
        uint32_t createdEpoch,
        uint32_t &surveyId);

    // Saved sessions are indexed newest first. GetSavedSession()
    // performs on-demand SD loading and integrity verification.
    static uint8_t GetSavedSessionCount();

    static const StoredWiFiMeasurementSessionIndex *
        GetSavedSessionIndex(uint8_t index);

    static const StoredWiFiMeasurementSession *
        GetSavedSession(uint8_t index);

    static bool SaveActiveSiteSurvey(
        uint32_t surveyId,
        const char *name,
        uint32_t createdEpoch);

    static bool LoadActiveSiteSurvey(
        StoredActiveSiteSurvey &survey);

    static bool ClearActiveSiteSurvey();

private:
    static constexpr uint8_t CurrentSessionFormatVersion = 7;
    static constexpr uint8_t EnumeratedSessionCapacity = 128;
    static constexpr uint8_t InvalidLoadedSessionIndex = 0xFF;
    

    enum class SessionReadResult : uint8_t
    {
        Success = 0,
        OpenFailed,
        ParseFailed,
        UnsupportedVersion,
        ChecksumMissing,
        ChecksumMismatch,
        SessionIdMismatch,
        IncompleteRecord
    };

    static bool available;
    static uint8_t detectedCardType;
    static uint64_t cardCapacityBytes;
    static uint64_t filesystemTotalBytes;
    static uint64_t filesystemUsedBytes;
    static StorageValidationResult validationResult;
    static bool externalReadOnlyAccessActive;

    static StoredWiFiMeasurementSessionIndex
        savedSessionIndex[MaxSavedSessions];

    static StoredWiFiMeasurementSession loadedSession;
    static uint8_t loadedSessionIndex;

    static uint8_t savedSessionCount;
    static uint32_t nextSessionId;
    static uint32_t nextSiteSurveyId;
    static StoredSiteSurveyIndex
        savedSiteSurveyIndex[
            MaxSavedSiteSurveys];

    static uint8_t savedSiteSurveyCount;

    static uint32_t nextFloorPlanId;

    static StoredFloorPlanIndex
        savedFloorPlanIndex[MaxSavedFloorPlans];

    static uint8_t savedFloorPlanCount;

    static FloorPlanImportImage
        floorPlanImportIndex[MaxFloorPlanImportImages];

    static uint8_t floorPlanImportCount;

    static uint32_t nextSiteSurveyPointId;

    static StoredSiteSurveyPointIndex
        savedSiteSurveyPointIndex[
            MaxSavedSiteSurveyPoints];

    static uint8_t savedSiteSurveyPointCount;

    static uint32_t nextPhysicalAccessPointId;

    static StoredPhysicalAccessPointIndex
        savedPhysicalAccessPointIndex[
            MaxSavedPhysicalAccessPoints];

    static uint8_t savedPhysicalAccessPointCount;

    static void LoadSiteSurveySequence();
    static void LoadFloorPlanSequence();
    static void LoadSiteSurveyPointSequence();
    static void LoadPhysicalAccessPointSequence();

    static bool EnsureDirectories();
    static bool CleanupStaleTemporaryFile();
    static void LoadMeasurementSessions();

    static bool WriteMeasurementSession(
        const StoredWiFiMeasurementSession &session);

    static SessionReadResult ReadMeasurementSession(
        uint32_t sessionId,
        StoredWiFiMeasurementSession &session);

    static SessionReadResult ParseMeasurementSession(
        fs::File &file,
        StoredWiFiMeasurementSession &session);

    static bool VerifyTemporarySession(
        const StoredWiFiMeasurementSession &expected);

    static bool WriteSummaryFields(
        fs::File &file,
        const StoredWiFiMeasurementSession &session);

    static const char *GetSessionReadResultText(
        SessionReadResult result);

    static uint32_t ExtractSessionId(
        const char *path);

    static void BuildSessionPath(
        uint32_t sessionId,
        char *buffer,
        size_t bufferSize);

    static void InsertSavedSessionIndex(
        const StoredWiFiMeasurementSession &session);

    static void RemoveIndexedSessionAt(
        uint8_t index);

    static void UpdateIndexMetadata(
        uint8_t index,
        const StoredWiFiMeasurementSession &session);

    static void InvalidateLoadedSession();

    static void RemoveSessionFile(
        uint32_t sessionId);

    static void QuarantineSessionFile(
        uint32_t sessionId,
        SessionReadResult result);

    static bool RunReadWriteValidation();

    static void SetFailure(
        StorageValidationResult result,
        const char *message);

    static constexpr uint8_t CurrentSiteSurveyFormatVersion = 1;

    static bool WriteSiteSurveyRecord(
        const StoredSiteSurvey &survey);

    static bool ReadSiteSurveyRecord(
        uint32_t surveyId,
        StoredSiteSurvey &survey);

    static void BuildSiteSurveyPath(
        uint32_t surveyId,
        char *buffer,
        size_t bufferSize);

    static constexpr uint8_t
        CurrentFloorPlanFormatVersion = 1;

    static bool WriteFloorPlanRecord(
        const StoredFloorPlan &floorPlan);

    static bool ReadFloorPlanRecord(
        uint32_t floorPlanId,
        StoredFloorPlan &floorPlan);

    static void BuildFloorPlanPath(
        uint32_t floorPlanId,
        char *buffer,
        size_t bufferSize);

    static bool IsSupportedFloorPlanImage(
        const char *path);

    static bool ReadFloorPlanImageDimensions(
        const char *path,
        uint16_t &width,
        uint16_t &height);

    static void BuildImportedFloorPlanName(
        const char *path,
        char *buffer,
        size_t bufferSize);

    static bool BuildRegisteredFloorPlanImagePath(
        uint32_t floorPlanId,
        const char *sourcePath,
        char *buffer,
        size_t bufferSize);

    static constexpr uint8_t
        CurrentSiteSurveyPointFormatVersion = 2;

    static constexpr uint8_t
        LegacySiteSurveyPointFormatVersion = 1;

    static bool WriteSiteSurveyPointRecord(
        const StoredSiteSurveyPoint &point,
        bool replaceExisting = false);

    static bool RecoverInterruptedSiteSurveyPointUpdates();

    static bool ReadSiteSurveyPointRecord(
        uint32_t pointId,
        StoredSiteSurveyPoint &point);

    static void BuildSiteSurveyPointPath(
        uint32_t pointId,
        char *buffer,
        size_t bufferSize);

    static void BuildSiteSurveyPointBackupPath(
        uint32_t pointId,
        char *buffer,
        size_t bufferSize);

    static constexpr uint8_t
        CurrentPhysicalAccessPointFormatVersion = 1;

    static bool WritePhysicalAccessPointRecord(
        const StoredPhysicalAccessPoint &accessPoint,
        bool replaceExisting = false);

    static bool RecoverInterruptedPhysicalAccessPointUpdates();

    static bool ReadPhysicalAccessPointRecord(
        uint32_t accessPointId,
        StoredPhysicalAccessPoint &accessPoint);

    static void BuildPhysicalAccessPointPath(
        uint32_t accessPointId,
        char *buffer,
        size_t bufferSize);

    static void BuildPhysicalAccessPointBackupPath(
        uint32_t accessPointId,
        char *buffer,
        size_t bufferSize);

    static constexpr uint8_t
        CurrentActiveSiteSurveyFormatVersion = 1;
};
