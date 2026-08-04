#pragma once

#include <stdint.h>

enum class StorageValidationResult : uint8_t
{
    NotRun = 0,
    Passed,
    CardUnavailable,
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
    static void Begin();

    static bool IsAvailable();
    static bool ValidationPassed();

    static StorageValidationResult GetValidationResult();
    static const char *GetValidationResultText();
    static const char *GetCardTypeText();

    static uint64_t GetCardCapacityBytes();
    static uint64_t GetFilesystemTotalBytes();
    static uint64_t GetFilesystemUsedBytes();
    static uint64_t GetFilesystemFreeBytes();

private:
    static bool available;
    static uint8_t detectedCardType;
    static uint64_t cardCapacityBytes;
    static uint64_t filesystemTotalBytes;
    static uint64_t filesystemUsedBytes;
    static StorageValidationResult validationResult;

    static bool RunReadWriteValidation();
    static void SetFailure(
        StorageValidationResult result,
        const char *message
    );
};
