#include "StorageService.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <cstring>

namespace
{
    constexpr const char *ValidationPath =
        "/sentinel_test.txt";

    constexpr const char ValidationRecord[] =
        "SentinelOS SD validation 10.15A\n";

    constexpr uint64_t BytesPerMegabyte =
        1024ULL * 1024ULL;
}

bool StorageService::available = false;
uint8_t StorageService::detectedCardType = CARD_NONE;
uint64_t StorageService::cardCapacityBytes = 0;
uint64_t StorageService::filesystemTotalBytes = 0;
uint64_t StorageService::filesystemUsedBytes = 0;
StorageValidationResult StorageService::validationResult =
    StorageValidationResult::NotRun;

void StorageService::Begin()
{
    available = false;
    detectedCardType = CARD_NONE;
    cardCapacityBytes = 0;
    filesystemTotalBytes = 0;
    filesystemUsedBytes = 0;
    validationResult = StorageValidationResult::NotRun;

    Serial.println("StorageService: Checking mounted SD card");

    // LilyGo_AMOLED::begin() initializes the onboard SD interface.
    // Do not call SD.begin() again here because that would reconfigure
    // the SPI bus already owned by the LilyGO hardware library.
    detectedCardType = SD.cardType();

    if (detectedCardType == CARD_NONE)
    {
        SetFailure(
            StorageValidationResult::CardUnavailable,
            "No mounted SD card detected"
        );
        return;
    }

    available = true;
    cardCapacityBytes = SD.cardSize();
    filesystemTotalBytes = SD.totalBytes();
    filesystemUsedBytes = SD.usedBytes();

    Serial.println("StorageService: SD card mounted");
    Serial.printf(
        "StorageService: Card type %s\n",
        GetCardTypeText()
    );
    Serial.printf(
        "StorageService: Card capacity %llu MB\n",
        static_cast<unsigned long long>(
            cardCapacityBytes / BytesPerMegabyte
        )
    );
    Serial.printf(
        "StorageService: Filesystem total %llu MB, used %llu MB, free %llu MB\n",
        static_cast<unsigned long long>(
            filesystemTotalBytes / BytesPerMegabyte
        ),
        static_cast<unsigned long long>(
            filesystemUsedBytes / BytesPerMegabyte
        ),
        static_cast<unsigned long long>(
            GetFilesystemFreeBytes() / BytesPerMegabyte
        )
    );

    RunReadWriteValidation();
}

bool StorageService::IsAvailable()
{
    return available;
}

bool StorageService::ValidationPassed()
{
    return validationResult == StorageValidationResult::Passed;
}

StorageValidationResult StorageService::GetValidationResult()
{
    return validationResult;
}

const char *StorageService::GetValidationResultText()
{
    switch (validationResult)
    {
        case StorageValidationResult::NotRun:
            return "Not run";
        case StorageValidationResult::Passed:
            return "Passed";
        case StorageValidationResult::CardUnavailable:
            return "Card unavailable";
        case StorageValidationResult::StaleFileCleanupFailed:
            return "Stale test file cleanup failed";
        case StorageValidationResult::FileCreateFailed:
            return "Test file creation failed";
        case StorageValidationResult::FileWriteFailed:
            return "Test file write failed";
        case StorageValidationResult::FileReadOpenFailed:
            return "Test file reopen failed";
        case StorageValidationResult::FileReadFailed:
            return "Test file read failed";
        case StorageValidationResult::ContentMismatch:
            return "Validation content mismatch";
        case StorageValidationResult::TestFileCleanupFailed:
            return "Test file cleanup failed";
        default:
            return "Unknown";
    }
}

const char *StorageService::GetCardTypeText()
{
    switch (detectedCardType)
    {
        case CARD_MMC:
            return "MMC";
        case CARD_SD:
            return "SDSC";
        case CARD_SDHC:
            return "SDHC/SDXC";
        case CARD_NONE:
            return "None";
        default:
            return "Unknown";
    }
}

uint64_t StorageService::GetCardCapacityBytes()
{
    return cardCapacityBytes;
}

uint64_t StorageService::GetFilesystemTotalBytes()
{
    return filesystemTotalBytes;
}

uint64_t StorageService::GetFilesystemUsedBytes()
{
    return filesystemUsedBytes;
}

uint64_t StorageService::GetFilesystemFreeBytes()
{
    if (filesystemUsedBytes >= filesystemTotalBytes)
    {
        return 0;
    }

    return filesystemTotalBytes - filesystemUsedBytes;
}

bool StorageService::RunReadWriteValidation()
{
    if (!available)
    {
        SetFailure(
            StorageValidationResult::CardUnavailable,
            "Read/write validation cannot run without a mounted card"
        );
        return false;
    }

    if (SD.exists(ValidationPath))
    {
        if (!SD.remove(ValidationPath))
        {
            SetFailure(
                StorageValidationResult::StaleFileCleanupFailed,
                "Could not remove stale validation file"
            );
            return false;
        }

        Serial.println(
            "StorageService: Removed stale validation file"
        );
    }

    File writeFile = SD.open(ValidationPath, FILE_WRITE);

    if (!writeFile)
    {
        SetFailure(
            StorageValidationResult::FileCreateFailed,
            "Could not create validation file"
        );
        return false;
    }

    Serial.println("StorageService: Validation file created");

    const size_t expectedLength =
        sizeof(ValidationRecord) - 1;

    const size_t bytesWritten = writeFile.write(
        reinterpret_cast<const uint8_t *>(ValidationRecord),
        expectedLength
    );

    writeFile.flush();
    writeFile.close();

    if (bytesWritten != expectedLength)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileWriteFailed,
            "Validation record was not written completely"
        );
        return false;
    }

    Serial.printf(
        "StorageService: Validation record written (%u bytes)\n",
        static_cast<unsigned int>(bytesWritten)
    );

    File readFile = SD.open(ValidationPath, FILE_READ);

    if (!readFile)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileReadOpenFailed,
            "Could not reopen validation file"
        );
        return false;
    }

    if (readFile.size() != expectedLength)
    {
        readFile.close();
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::ContentMismatch,
            "Validation file length did not match"
        );
        return false;
    }

    char readBuffer[sizeof(ValidationRecord)] = {};
    const size_t bytesRead = readFile.read(
        reinterpret_cast<uint8_t *>(readBuffer),
        expectedLength
    );

    readFile.close();

    if (bytesRead != expectedLength)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileReadFailed,
            "Validation record was not read completely"
        );
        return false;
    }

    if (std::memcmp(
            readBuffer,
            ValidationRecord,
            expectedLength
        ) != 0)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::ContentMismatch,
            "Validation record did not match written content"
        );
        return false;
    }

    Serial.println(
        "StorageService: Validation record verified"
    );

    if (!SD.remove(ValidationPath))
    {
        SetFailure(
            StorageValidationResult::TestFileCleanupFailed,
            "Validated file could not be deleted"
        );
        return false;
    }

    Serial.println("StorageService: Validation file removed");

    validationResult = StorageValidationResult::Passed;
    Serial.println("StorageService: SD validation PASSED");

    return true;
}

void StorageService::SetFailure(
    StorageValidationResult result,
    const char *message
)
{
    validationResult = result;

    Serial.printf(
        "StorageService: SD validation FAILED - %s\n",
        message == nullptr ? GetValidationResultText() : message
    );
}
