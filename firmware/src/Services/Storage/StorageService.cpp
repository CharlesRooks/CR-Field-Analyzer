#include "StorageService.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
    constexpr const char *RootDirectory =
        "/sentinel";

    constexpr const char *SessionsDirectory =
        "/sentinel/sessions";

    constexpr const char *TemporarySessionPath =
        "/sentinel/sessions/session.tmp";

    constexpr const char *ValidationPath =
        "/sentinel_test.txt";

    constexpr const char ValidationRecord[] =
        "SentinelOS SD validation 10.15A\n";

    constexpr uint64_t BytesPerMegabyte =
        1024ULL * 1024ULL;

    bool ParseUnsigned(
        const String &value,
        uint32_t &result)
    {
        if (value.length() == 0)
        {
            return false;
        }

        char *end = nullptr;
        const unsigned long parsed =
            std::strtoul(value.c_str(), &end, 10);

        if (end == value.c_str() ||
            *end != '\0')
        {
            return false;
        }

        result = static_cast<uint32_t>(parsed);
        return true;
    }

    bool ParseBoolean(
        const String &value,
        bool &result)
    {
        uint32_t parsed = 0;

        if (!ParseUnsigned(value, parsed) ||
            parsed > 1)
        {
            return false;
        }

        result = parsed == 1;
        return true;
    }

    void SortDescending(
        uint32_t *values,
        uint8_t count)
    {
        for (uint8_t index = 1;
             index < count;
             ++index)
        {
            const uint32_t current = values[index];
            int16_t compare =
                static_cast<int16_t>(index) - 1;

            while (compare >= 0 &&
                   values[compare] < current)
            {
                values[compare + 1] =
                    values[compare];
                --compare;
            }

            values[compare + 1] = current;
        }
    }
}

bool StorageService::available = false;
uint8_t StorageService::detectedCardType = CARD_NONE;
uint64_t StorageService::cardCapacityBytes = 0;
uint64_t StorageService::filesystemTotalBytes = 0;
uint64_t StorageService::filesystemUsedBytes = 0;
StorageValidationResult StorageService::validationResult =
    StorageValidationResult::NotRun;

StoredWiFiMeasurementSession
    StorageService::savedSessions[
        StorageService::MaxSavedSessions] = {};

uint8_t StorageService::savedSessionCount = 0;
uint32_t StorageService::nextSessionId = 1;

void StorageService::Begin()
{
    available = false;
    detectedCardType = CARD_NONE;
    cardCapacityBytes = 0;
    filesystemTotalBytes = 0;
    filesystemUsedBytes = 0;
    validationResult = StorageValidationResult::NotRun;
    savedSessionCount = 0;
    nextSessionId = 1;

    for (uint8_t index = 0;
         index < MaxSavedSessions;
         ++index)
    {
        savedSessions[index] =
            StoredWiFiMeasurementSession{};
    }

    Serial.println(
        "StorageService: Checking mounted SD card");

    // LilyGo_AMOLED::begin() owns initialization of the onboard
    // SD interface. Reusing the mounted global SD filesystem
    // avoids a second SPI configuration.
    detectedCardType = SD.cardType();

    if (detectedCardType == CARD_NONE)
    {
        SetFailure(
            StorageValidationResult::CardUnavailable,
            "No mounted SD card detected");
        return;
    }

    available = true;
    cardCapacityBytes = SD.cardSize();
    filesystemTotalBytes = SD.totalBytes();
    filesystemUsedBytes = SD.usedBytes();

    Serial.println("StorageService: SD card mounted");
    Serial.printf(
        "StorageService: Card type %s\n",
        GetCardTypeText());
    Serial.printf(
        "StorageService: Card capacity %llu MB\n",
        static_cast<unsigned long long>(
            cardCapacityBytes / BytesPerMegabyte));
    Serial.printf(
        "StorageService: Filesystem total %llu MB, used %llu MB, free %llu MB\n",
        static_cast<unsigned long long>(
            filesystemTotalBytes / BytesPerMegabyte),
        static_cast<unsigned long long>(
            filesystemUsedBytes / BytesPerMegabyte),
        static_cast<unsigned long long>(
            GetFilesystemFreeBytes() / BytesPerMegabyte));

    if (!EnsureDirectories())
    {
        available = false;
        SetFailure(
            StorageValidationResult::DirectoryCreateFailed,
            "Could not create SentinelOS storage directories");
        return;
    }

    LoadMeasurementSessions();

    Serial.println(
        "StorageService: Startup validation skipped; "
        "run-once validation already completed");
}

bool StorageService::RunValidation()
{
    return RunReadWriteValidation();
}

bool StorageService::IsAvailable()
{
    return available;
}

bool StorageService::ValidationPassed()
{
    return validationResult ==
        StorageValidationResult::Passed;
}

StorageValidationResult
StorageService::GetValidationResult()
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
        case StorageValidationResult::DirectoryCreateFailed:
            return "Directory creation failed";
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
    if (filesystemUsedBytes >=
        filesystemTotalBytes)
    {
        return 0;
    }

    return filesystemTotalBytes -
        filesystemUsedBytes;
}

bool StorageService::SaveMeasurementSummary(
    const WiFiMeasurementSummary &summary,
    uint32_t completedAtMs)
{
    if (!available || !summary.available)
    {
        Serial.println(
            "StorageService: Session save skipped - "
            "storage or summary unavailable");
        return false;
    }

    if (!EnsureDirectories())
    {
        Serial.println(
            "StorageService: Session save failed - "
            "storage directories unavailable");
        return false;
    }

    StoredWiFiMeasurementSession session{};
    session.available = true;
    session.sessionId = nextSessionId;
    session.completedAtMs = completedAtMs;
    session.summary = summary;

    if (!WriteMeasurementSession(session))
    {
        Serial.printf(
            "StorageService: Session %lu save failed\n",
            static_cast<unsigned long>(
                session.sessionId));
        return false;
    }

    uint32_t displacedSessionId = 0;

    if (savedSessionCount >= MaxSavedSessions)
    {
        displacedSessionId =
            savedSessions[
                MaxSavedSessions - 1]
                .sessionId;
    }

    InsertSavedSession(session);

    if (displacedSessionId != 0 &&
        displacedSessionId != session.sessionId)
    {
        RemoveSessionFile(displacedSessionId);
    }

    ++nextSessionId;

    if (nextSessionId == 0)
    {
        nextSessionId = 1;
    }

    filesystemUsedBytes = SD.usedBytes();

    Serial.printf(
        "StorageService: Session %lu saved, "
        "%u/%u retained\n",
        static_cast<unsigned long>(
            session.sessionId),
        savedSessionCount,
        MaxSavedSessions);

    return true;
}

uint8_t StorageService::GetSavedSessionCount()
{
    return savedSessionCount;
}

const StoredWiFiMeasurementSession *
StorageService::GetSavedSession(uint8_t index)
{
    if (index >= savedSessionCount)
    {
        return nullptr;
    }

    return &savedSessions[index];
}

bool StorageService::EnsureDirectories()
{
    if (!available)
    {
        return false;
    }

    if (!SD.exists(RootDirectory) &&
        !SD.mkdir(RootDirectory))
    {
        return false;
    }

    if (!SD.exists(SessionsDirectory) &&
        !SD.mkdir(SessionsDirectory))
    {
        return false;
    }

    return true;
}

void StorageService::LoadMeasurementSessions()
{
    uint32_t sessionIds[
        EnumeratedSessionCapacity] = {};

    uint8_t enumeratedCount = 0;
    uint32_t maximumSessionId = 0;

    File directory = SD.open(SessionsDirectory);

    if (!directory || !directory.isDirectory())
    {
        Serial.println(
            "StorageService: Session directory unavailable");
        return;
    }

    File entry = directory.openNextFile();

    while (entry)
    {
        if (!entry.isDirectory())
        {
            const uint32_t sessionId =
                ExtractSessionId(entry.name());

            if (sessionId != 0)
            {
                if (enumeratedCount <
                    EnumeratedSessionCapacity)
                {
                    sessionIds[enumeratedCount] =
                        sessionId;
                    ++enumeratedCount;
                }

                if (sessionId > maximumSessionId)
                {
                    maximumSessionId = sessionId;
                }
            }
        }

        entry.close();
        entry = directory.openNextFile();
    }

    directory.close();

    SortDescending(
        sessionIds,
        enumeratedCount);

    uint32_t keptIds[MaxSavedSessions] = {};
    uint8_t keptCount = 0;

    for (uint8_t index = 0;
         index < enumeratedCount &&
         keptCount < MaxSavedSessions;
         ++index)
    {
        StoredWiFiMeasurementSession session{};

        if (!ReadMeasurementSession(
                sessionIds[index],
                session))
        {
            Serial.printf(
                "StorageService: Ignored unreadable "
                "session %lu\n",
                static_cast<unsigned long>(
                    sessionIds[index]));
            continue;
        }

        savedSessions[keptCount] = session;
        keptIds[keptCount] = session.sessionId;
        ++keptCount;
    }

    savedSessionCount = keptCount;

    // Remove valid session files outside the rolling retention
    // window. Unrecognized files in the directory are preserved.
    for (uint8_t index = 0;
         index < enumeratedCount;
         ++index)
    {
        if (!IsSessionIdKept(
                sessionIds[index],
                keptIds,
                keptCount))
        {
            RemoveSessionFile(sessionIds[index]);
        }
    }

    nextSessionId = maximumSessionId + 1;

    if (nextSessionId == 0)
    {
        nextSessionId = 1;
    }

    Serial.printf(
        "StorageService: Restored %u saved "
        "measurement session%s; next ID %lu\n",
        savedSessionCount,
        savedSessionCount == 1 ? "" : "s",
        static_cast<unsigned long>(nextSessionId));
}

bool StorageService::WriteMeasurementSession(
    const StoredWiFiMeasurementSession &session)
{
    if (SD.exists(TemporarySessionPath) &&
        !SD.remove(TemporarySessionPath))
    {
        Serial.println(
            "StorageService: Could not remove stale "
            "session temporary file");
        return false;
    }

    File file =
        SD.open(TemporarySessionPath, FILE_WRITE);

    if (!file)
    {
        return false;
    }

    const bool written =
        WriteSummaryFields(file, session);

    file.flush();
    file.close();

    if (!written)
    {
        SD.remove(TemporarySessionPath);
        return false;
    }

    char finalPath[64];
    BuildSessionPath(
        session.sessionId,
        finalPath,
        sizeof(finalPath));

    if (SD.exists(finalPath) &&
        !SD.remove(finalPath))
    {
        SD.remove(TemporarySessionPath);
        return false;
    }

    if (!SD.rename(
            TemporarySessionPath,
            finalPath))
    {
        SD.remove(TemporarySessionPath);
        return false;
    }

    return true;
}

bool StorageService::ReadMeasurementSession(
    uint32_t sessionId,
    StoredWiFiMeasurementSession &session)
{
    char path[64];
    BuildSessionPath(
        sessionId,
        path,
        sizeof(path));

    File file = SD.open(path, FILE_READ);

    if (!file)
    {
        return false;
    }

    const bool parsed =
        ParseMeasurementSession(file, session);

    file.close();

    return parsed &&
        session.available &&
        session.sessionId == sessionId &&
        session.summary.available;
}

bool StorageService::ParseMeasurementSession(
    fs::File &file,
    StoredWiFiMeasurementSession &session)
{
    session = StoredWiFiMeasurementSession{};

    uint32_t version = 0;
    bool versionSeen = false;
    bool idSeen = false;
    bool scansSeen = false;
    bool bestChannelSeen = false;

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0 ||
            line.startsWith("#"))
        {
            continue;
        }

        const int separator = line.indexOf('=');

        if (separator <= 0)
        {
            continue;
        }

        const String key =
            line.substring(0, separator);
        const String value =
            line.substring(separator + 1);

        uint32_t parsed = 0;

        if (key == "version")
        {
            if (!ParseUnsigned(value, version))
                return false;
            versionSeen = true;
        }
        else if (key == "session_id")
        {
            if (!ParseUnsigned(value, session.sessionId))
                return false;
            idSeen = true;
        }
        else if (key == "completed_at_ms")
        {
            if (!ParseUnsigned(value, session.completedAtMs))
                return false;
        }
        else if (key == "completed_scans")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.completedScanCount =
                static_cast<uint8_t>(parsed);
            scansSeen = true;
        }
        else if (key == "network_count")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.networkCount =
                static_cast<uint8_t>(parsed);
        }
        else if (key == "occupied_channels")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.occupiedChannelCount =
                static_cast<uint8_t>(parsed);
        }
        else if (key == "best_channel")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.recommendation.bestChannel =
                static_cast<uint8_t>(parsed);
            bestChannelSeen = true;
        }
        else if (key == "best_score")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 65535)
                return false;
            session.summary.recommendation.bestScore =
                static_cast<uint16_t>(parsed);
        }
        else if (key == "second_best_score")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 65535)
                return false;
            session.summary.recommendation.secondBestScore =
                static_cast<uint16_t>(parsed);
        }
        else if (key == "score_margin")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 65535)
                return false;
            session.summary.recommendation.scoreMargin =
                static_cast<uint16_t>(parsed);
        }
        else if (key == "comparable_count")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.recommendation.comparableCount =
                static_cast<uint8_t>(parsed);
        }
        else if (key == "history_samples")
        {
            if (!ParseUnsigned(value, parsed) || parsed > 255)
                return false;
            session.summary.recommendation.historySampleCount =
                static_cast<uint8_t>(parsed);
        }
        else if (key == "confidence")
        {
            if (!ParseUnsigned(value, parsed) ||
                parsed > static_cast<uint32_t>(
                    WiFiRecommendationConfidence::High))
                return false;
            session.summary.recommendation.confidence =
                static_cast<WiFiRecommendationConfidence>(parsed);
        }
        else if (key == "unique")
        {
            if (!ParseBoolean(
                    value,
                    session.summary.recommendation.unique))
                return false;
        }
        else if (key.startsWith("candidate_"))
        {
            const uint8_t index =
                static_cast<uint8_t>(
                    key.substring(10).toInt());

            if (index >=
                WiFiMeasurementSummary::CandidateCapacity)
                continue;

            unsigned int channel = 0;
            unsigned int latest = 0;
            unsigned int average = 0;
            unsigned int congestion = 0;
            unsigned int recommended = 0;
            unsigned int comparable = 0;

            if (std::sscanf(
                    value.c_str(),
                    "%u,%u,%u,%u,%u,%u",
                    &channel,
                    &latest,
                    &average,
                    &congestion,
                    &recommended,
                    &comparable) != 6)
                return false;

            WiFiChannelAssessment &assessment =
                session.summary.candidates[index];
            assessment.channel =
                static_cast<uint8_t>(channel);
            assessment.latestScore =
                static_cast<uint16_t>(latest);
            assessment.congestionScore =
                static_cast<uint16_t>(average);
            assessment.congestion =
                static_cast<WiFiCongestionLevel>(congestion);
            assessment.recommended = recommended != 0;
            assessment.comparable = comparable != 0;
        }
        else if (key.startsWith("channel_"))
        {
            const uint8_t channelIndex =
                static_cast<uint8_t>(
                    key.substring(8).toInt());

            if (channelIndex >=
                WiFiMeasurementSummary::ChannelCapacity)
                continue;

            unsigned int channel = 0;
            unsigned int count = 0;
            long strongest = -127;
            long average = -127;

            if (std::sscanf(
                    value.c_str(),
                    "%u,%u,%ld,%ld",
                    &channel,
                    &count,
                    &strongest,
                    &average) != 4)
                return false;

            WiFiChannelInfo &info =
                session.summary.channels[channelIndex];
            info.channel =
                static_cast<uint8_t>(channel);
            info.networkCount =
                static_cast<uint8_t>(count);
            info.strongestRssi =
                static_cast<int32_t>(strongest);
            info.averageRssi =
                static_cast<int32_t>(average);
        }
    }

    if (!versionSeen || version != 1 ||
        !idSeen || !scansSeen ||
        !bestChannelSeen)
    {
        return false;
    }

    session.available = true;
    session.summary.available = true;
    return true;
}

bool StorageService::WriteSummaryFields(
    fs::File &file,
    const StoredWiFiMeasurementSession &session)
{
    const WiFiMeasurementSummary &summary =
        session.summary;

    if (!file.printf("version=1\n") ||
        !file.printf(
            "session_id=%lu\n",
            static_cast<unsigned long>(
                session.sessionId)) ||
        !file.printf(
            "completed_at_ms=%lu\n",
            static_cast<unsigned long>(
                session.completedAtMs)) ||
        !file.printf(
            "completed_scans=%u\n",
            summary.completedScanCount) ||
        !file.printf(
            "network_count=%u\n",
            summary.networkCount) ||
        !file.printf(
            "occupied_channels=%u\n",
            summary.occupiedChannelCount) ||
        !file.printf(
            "best_channel=%u\n",
            summary.recommendation.bestChannel) ||
        !file.printf(
            "best_score=%u\n",
            summary.recommendation.bestScore) ||
        !file.printf(
            "second_best_score=%u\n",
            summary.recommendation.secondBestScore) ||
        !file.printf(
            "score_margin=%u\n",
            summary.recommendation.scoreMargin) ||
        !file.printf(
            "comparable_count=%u\n",
            summary.recommendation.comparableCount) ||
        !file.printf(
            "history_samples=%u\n",
            summary.recommendation.historySampleCount) ||
        !file.printf(
            "confidence=%u\n",
            static_cast<unsigned int>(
                summary.recommendation.confidence)) ||
        !file.printf(
            "unique=%u\n",
            summary.recommendation.unique ? 1 : 0))
    {
        return false;
    }

    for (uint8_t index = 0;
         index <
             WiFiMeasurementSummary::CandidateCapacity;
         ++index)
    {
        const WiFiChannelAssessment &assessment =
            summary.candidates[index];

        if (!file.printf(
                "candidate_%u=%u,%u,%u,%u,%u,%u\n",
                index,
                assessment.channel,
                assessment.latestScore,
                assessment.congestionScore,
                static_cast<unsigned int>(
                    assessment.congestion),
                assessment.recommended ? 1 : 0,
                assessment.comparable ? 1 : 0))
        {
            return false;
        }
    }

    for (uint8_t channel = 0;
         channel <
             WiFiMeasurementSummary::ChannelCapacity;
         ++channel)
    {
        const WiFiChannelInfo &info =
            summary.channels[channel];

        if (!file.printf(
                "channel_%u=%u,%u,%ld,%ld\n",
                channel,
                info.channel,
                info.networkCount,
                static_cast<long>(
                    info.strongestRssi),
                static_cast<long>(
                    info.averageRssi)))
        {
            return false;
        }
    }

    return true;
}

uint32_t StorageService::ExtractSessionId(
    const char *path)
{
    if (path == nullptr)
    {
        return 0;
    }

    const char *name =
        std::strrchr(path, '/');

    name = name == nullptr ? path : name + 1;

    unsigned long sessionId = 0;
    char trailing = '\0';

    if (std::sscanf(
            name,
            "session_%lu.txt%c",
            &sessionId,
            &trailing) != 1)
    {
        return 0;
    }

    if (sessionId == 0)
    {
        return 0;
    }

    return static_cast<uint32_t>(sessionId);
}

void StorageService::BuildSessionPath(
    uint32_t sessionId,
    char *buffer,
    size_t bufferSize)
{
    if (buffer == nullptr ||
        bufferSize == 0)
    {
        return;
    }

    std::snprintf(
        buffer,
        bufferSize,
        "%s/session_%06lu.txt",
        SessionsDirectory,
        static_cast<unsigned long>(sessionId));
}

void StorageService::InsertSavedSession(
    const StoredWiFiMeasurementSession &session)
{
    const uint8_t upperIndex =
        savedSessionCount < MaxSavedSessions
            ? savedSessionCount
            : MaxSavedSessions - 1;

    for (uint8_t index = upperIndex;
         index > 0;
         --index)
    {
        savedSessions[index] =
            savedSessions[index - 1];
    }

    savedSessions[0] = session;

    if (savedSessionCount < MaxSavedSessions)
    {
        ++savedSessionCount;
    }
}

void StorageService::RemoveSessionFile(
    uint32_t sessionId)
{
    char path[64];
    BuildSessionPath(
        sessionId,
        path,
        sizeof(path));

    if (SD.exists(path) &&
        !SD.remove(path))
    {
        Serial.printf(
            "StorageService: Warning - could not "
            "remove old session %lu\n",
            static_cast<unsigned long>(sessionId));
    }
}

bool StorageService::IsSessionIdKept(
    uint32_t sessionId,
    const uint32_t *keptIds,
    uint8_t keptCount)
{
    for (uint8_t index = 0;
         index < keptCount;
         ++index)
    {
        if (keptIds[index] == sessionId)
        {
            return true;
        }
    }

    return false;
}

bool StorageService::RunReadWriteValidation()
{
    if (!available)
    {
        SetFailure(
            StorageValidationResult::CardUnavailable,
            "Read/write validation cannot run without a mounted card");
        return false;
    }

    if (SD.exists(ValidationPath) &&
        !SD.remove(ValidationPath))
    {
        SetFailure(
            StorageValidationResult::StaleFileCleanupFailed,
            "Could not remove stale validation file");
        return false;
    }

    File writeFile =
        SD.open(ValidationPath, FILE_WRITE);

    if (!writeFile)
    {
        SetFailure(
            StorageValidationResult::FileCreateFailed,
            "Could not create validation file");
        return false;
    }

    const size_t expectedLength =
        sizeof(ValidationRecord) - 1;

    const size_t bytesWritten =
        writeFile.write(
            reinterpret_cast<const uint8_t *>(
                ValidationRecord),
            expectedLength);

    writeFile.flush();
    writeFile.close();

    if (bytesWritten != expectedLength)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileWriteFailed,
            "Validation record was not written completely");
        return false;
    }

    File readFile =
        SD.open(ValidationPath, FILE_READ);

    if (!readFile)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileReadOpenFailed,
            "Could not reopen validation file");
        return false;
    }

    char readBuffer[
        sizeof(ValidationRecord)] = {};

    const size_t bytesRead =
        readFile.read(
            reinterpret_cast<uint8_t *>(readBuffer),
            expectedLength);

    readFile.close();

    if (bytesRead != expectedLength)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::FileReadFailed,
            "Validation record was not read completely");
        return false;
    }

    if (std::memcmp(
            readBuffer,
            ValidationRecord,
            expectedLength) != 0)
    {
        SD.remove(ValidationPath);
        SetFailure(
            StorageValidationResult::ContentMismatch,
            "Validation record did not match written content");
        return false;
    }

    if (!SD.remove(ValidationPath))
    {
        SetFailure(
            StorageValidationResult::TestFileCleanupFailed,
            "Validated file could not be deleted");
        return false;
    }

    validationResult =
        StorageValidationResult::Passed;

    Serial.println(
        "StorageService: SD validation PASSED");

    return true;
}

void StorageService::SetFailure(
    StorageValidationResult result,
    const char *message)
{
    validationResult = result;

    Serial.printf(
        "StorageService: %s - %s\n",
        result == StorageValidationResult::CardUnavailable ||
                result == StorageValidationResult::DirectoryCreateFailed
            ? "Storage unavailable"
            : "SD validation FAILED",
        message == nullptr
            ? GetValidationResultText()
            : message);
}
