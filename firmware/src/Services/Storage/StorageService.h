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
    static constexpr uint8_t MaxSavedSessions = 5;

    static void Begin();

    // Retains the Milestone 10.15A diagnostic without running a
    // destructive write/delete test during every normal startup.
    static bool RunValidation();

    static bool IsAvailable();
    static bool ValidationPassed();

    static StorageValidationResult GetValidationResult();
    static const char *GetValidationResultText();
    static const char *GetCardTypeText();

    static uint64_t GetCardCapacityBytes();
    static uint64_t GetFilesystemTotalBytes();
    static uint64_t GetFilesystemUsedBytes();
    static uint64_t GetFilesystemFreeBytes();

    static bool SaveMeasurementSummary(
        const WiFiMeasurementSummary &summary,
        uint32_t completedAtMs);

    // Saved sessions are exposed newest first.
    static uint8_t GetSavedSessionCount();

    static const StoredWiFiMeasurementSession *
        GetSavedSession(uint8_t index);

private:
    static constexpr uint8_t CurrentSessionFormatVersion = 2;
    static constexpr uint8_t EnumeratedSessionCapacity = 64;

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

    static StoredWiFiMeasurementSession
        savedSessions[MaxSavedSessions];

    static uint8_t savedSessionCount;
    static uint32_t nextSessionId;

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

    static void InsertSavedSession(
        const StoredWiFiMeasurementSession &session);

    static void RemoveSessionFile(
        uint32_t sessionId);

    static void QuarantineSessionFile(
        uint32_t sessionId,
        SessionReadResult result);

    static bool IsSessionIdKept(
        uint32_t sessionId,
        const uint32_t *keptIds,
        uint8_t keptCount);

    static bool RunReadWriteValidation();

    static void SetFailure(
        StorageValidationResult result,
        const char *message);
};
