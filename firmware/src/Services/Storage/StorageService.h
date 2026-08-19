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

    // While USB Mass Storage has the SD card exposed to a host,
    // SentinelOS must not modify the filesystem.
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
    static constexpr uint8_t CurrentSessionFormatVersion = 6;
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
    static void LoadSiteSurveySequence();

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

    static void BuildSiteSurveyPath(
        uint32_t surveyId,
        char *buffer,
        size_t bufferSize);

    static constexpr uint8_t
        CurrentActiveSiteSurveyFormatVersion = 1;
};
