#include "StorageService.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <cstdarg>
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

    constexpr uint32_t Crc32Initial =
        0xFFFFFFFFUL;

    bool TryExtractSessionFileId(
        const char *path,
        const char *requiredExtension,
        uint32_t &sessionId)
    {
        sessionId = 0;

        if (path == nullptr ||
            requiredExtension == nullptr)
        {
            return false;
        }

        const char *name =
            std::strrchr(path, '/');

        name = name == nullptr ? path : name + 1;

        constexpr const char Prefix[] =
            "session_";
        constexpr size_t PrefixLength =
            sizeof(Prefix) - 1;

        if (std::strncmp(
                name,
                Prefix,
                PrefixLength) != 0)
        {
            return false;
        }

        const char *numberStart =
            name + PrefixLength;

        char *numberEnd = nullptr;
        const unsigned long parsed =
            std::strtoul(
                numberStart,
                &numberEnd,
                10);

        if (numberEnd == numberStart ||
            parsed == 0 ||
            std::strcmp(
                numberEnd,
                requiredExtension) != 0)
        {
            return false;
        }

        sessionId =
            static_cast<uint32_t>(parsed);
        return true;
    }

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

    bool ParseHexUnsigned(
        const String &value,
        uint32_t &result)
    {
        if (value.length() == 0 ||
            value.length() > 8)
        {
            return false;
        }

        char *end = nullptr;
        const unsigned long parsed =
            std::strtoul(value.c_str(), &end, 16);

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

    uint32_t UpdateCrc32(
        uint32_t crc,
        const uint8_t *data,
        size_t length)
    {
        for (size_t index = 0;
             index < length;
             ++index)
        {
            crc ^= data[index];

            for (uint8_t bit = 0;
                 bit < 8;
                 ++bit)
            {
                const uint32_t mask =
                    static_cast<uint32_t>(
                        -static_cast<int32_t>(crc & 1U));

                crc = (crc >> 1U) ^
                    (0xEDB88320UL & mask);
            }
        }

        return crc;
    }

    uint32_t UpdateCrc32(
        uint32_t crc,
        const String &value)
    {
        return UpdateCrc32(
            crc,
            reinterpret_cast<const uint8_t *>(
                value.c_str()),
            value.length());
    }

    bool WriteCrcLine(
        fs::File &file,
        uint32_t &crc,
        const char *format,
        ...)
    {
        char line[128];

        va_list arguments;
        va_start(arguments, format);
        const int length = std::vsnprintf(
            line,
            sizeof(line),
            format,
            arguments);
        va_end(arguments);

        if (length <= 0 ||
            static_cast<size_t>(length) >=
                sizeof(line))
        {
            return false;
        }

        const size_t bytesWritten = file.write(
            reinterpret_cast<const uint8_t *>(line),
            static_cast<size_t>(length));

        if (bytesWritten !=
            static_cast<size_t>(length))
        {
            return false;
        }

        crc = UpdateCrc32(
            crc,
            reinterpret_cast<const uint8_t *>(line),
            static_cast<size_t>(length));

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

    CleanupStaleTemporaryFile();
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
    uint32_t completedAtMs,
    uint32_t capturedEpoch,
    const char *capturedLocal)
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
    session.formatVersion =
        CurrentSessionFormatVersion;
    session.integrityVerified = true;
    session.sessionId = nextSessionId;
    session.capturedTimeValid =
        capturedEpoch != 0 &&
        capturedLocal != nullptr &&
        capturedLocal[0] != '\0';
    session.capturedEpoch =
        session.capturedTimeValid
            ? capturedEpoch
            : 0;

    if (session.capturedTimeValid)
    {
        std::strncpy(
            session.capturedLocal,
            capturedLocal,
            StoredWiFiMeasurementSession::
                CapturedLocalCapacity - 1);

        session.capturedLocal[
            StoredWiFiMeasurementSession::
                CapturedLocalCapacity - 1] = '\0';
    }
    else
    {
        std::strncpy(
            session.capturedLocal,
            "unavailable",
            StoredWiFiMeasurementSession::
                CapturedLocalCapacity - 1);

        session.capturedLocal[
            StoredWiFiMeasurementSession::
                CapturedLocalCapacity - 1] = '\0';
    }

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

    if (session.capturedTimeValid)
    {
        Serial.printf(
            "StorageService: Session %lu captured %s\n",
            static_cast<unsigned long>(
                session.sessionId),
            session.capturedLocal);
    }
    else
    {
        Serial.printf(
            "StorageService: Session %lu capture time "
            "unavailable\n",
            static_cast<unsigned long>(
                session.sessionId));
    }

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

bool StorageService::CleanupStaleTemporaryFile()
{
    if (!SD.exists(TemporarySessionPath))
    {
        return true;
    }

    Serial.println(
        "StorageService: Found abandoned session "
        "temporary file");

    if (!SD.remove(TemporarySessionPath))
    {
        Serial.println(
            "StorageService: Warning - abandoned "
            "temporary file could not be removed");
        return false;
    }

    Serial.println(
        "StorageService: Abandoned session temporary "
        "file removed");

    return true;
}

void StorageService::LoadMeasurementSessions()
{
    uint32_t sessionIds[
        EnumeratedSessionCapacity] = {};

    uint8_t enumeratedCount = 0;
    uint32_t maximumSessionId = 0;
    bool enumerationTruncated = false;

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
                else
                {
                    enumerationTruncated = true;
                }

                if (sessionId > maximumSessionId)
                {
                    maximumSessionId = sessionId;
                }
            }
            else
            {
                // Quarantined records must not be loaded as active
                // .txt sessions, but their IDs still count so a
                // future save never reuses a quarantined identifier.
                uint32_t quarantinedSessionId = 0;

                if (TryExtractSessionFileId(
                        entry.name(),
                        ".bad",
                        quarantinedSessionId) &&
                    quarantinedSessionId >
                        maximumSessionId)
                {
                    maximumSessionId =
                        quarantinedSessionId;
                }
            }
        }

        entry.close();
        entry = directory.openNextFile();
    }

    directory.close();

    if (enumerationTruncated)
    {
        Serial.printf(
            "StorageService: Warning - session index "
            "limited to %u files\n",
            EnumeratedSessionCapacity);
    }

    SortDescending(
        sessionIds,
        enumeratedCount);

    uint32_t keptIds[MaxSavedSessions] = {};
    uint8_t keptCount = 0;

    uint32_t validIds[
        EnumeratedSessionCapacity] = {};
    uint8_t validCount = 0;
    uint8_t skippedCount = 0;

    for (uint8_t index = 0;
         index < enumeratedCount;
         ++index)
    {
        StoredWiFiMeasurementSession session{};

        const SessionReadResult result =
            ReadMeasurementSession(
                sessionIds[index],
                session);

        if (result != SessionReadResult::Success)
        {
            ++skippedCount;

            Serial.printf(
                "StorageService: Session %lu skipped - %s\n",
                static_cast<unsigned long>(
                    sessionIds[index]),
                GetSessionReadResultText(result));

            QuarantineSessionFile(
                sessionIds[index],
                result);
            continue;
        }

        validIds[validCount] = session.sessionId;
        ++validCount;

        if (session.integrityVerified)
        {
            Serial.printf(
                "StorageService: Session %lu verified "
                "(format v%u, CRC32 valid)\n",
                static_cast<unsigned long>(
                    session.sessionId),
                session.formatVersion);
        }
        else
        {
            Serial.printf(
                "StorageService: Session %lu loaded as "
                "legacy format v%u without checksum\n",
                static_cast<unsigned long>(
                    session.sessionId),
                session.formatVersion);
        }

        if (keptCount < MaxSavedSessions)
        {
            savedSessions[keptCount] = session;
            keptIds[keptCount] = session.sessionId;
            ++keptCount;
        }
    }

    savedSessionCount = keptCount;

    // Remove only verified/parseable records outside the rolling
    // retention window. Corrupt files are preserved for diagnosis
    // and skipped safely on future boots.
    for (uint8_t index = 0;
         index < validCount;
         ++index)
    {
        if (!IsSessionIdKept(
                validIds[index],
                keptIds,
                keptCount))
        {
            RemoveSessionFile(validIds[index]);
        }
    }

    nextSessionId = maximumSessionId + 1;

    if (nextSessionId == 0)
    {
        nextSessionId = 1;
    }

    Serial.printf(
        "StorageService: Restored %u valid saved "
        "measurement session%s; %u invalid skipped; "
        "next ID %lu\n",
        savedSessionCount,
        savedSessionCount == 1 ? "" : "s",
        skippedCount,
        static_cast<unsigned long>(nextSessionId));
}

bool StorageService::WriteMeasurementSession(
    const StoredWiFiMeasurementSession &session)
{
    if (SD.exists(TemporarySessionPath) &&
        !SD.remove(TemporarySessionPath))
    {
        Serial.println(
            "StorageService: Session save failed - "
            "stale temporary file could not be removed");
        return false;
    }

    File file =
        SD.open(TemporarySessionPath, FILE_WRITE);

    if (!file)
    {
        Serial.println(
            "StorageService: Session save failed - "
            "temporary file could not be created");
        return false;
    }

    const bool written =
        WriteSummaryFields(file, session);

    file.flush();
    file.close();

    if (!written)
    {
        SD.remove(TemporarySessionPath);
        Serial.println(
            "StorageService: Session save failed - "
            "record was not written completely");
        return false;
    }

    if (!VerifyTemporarySession(session))
    {
        SD.remove(TemporarySessionPath);
        Serial.println(
            "StorageService: Session save failed - "
            "temporary record verification failed");
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
        Serial.println(
            "StorageService: Session save failed - "
            "existing destination could not be replaced");
        return false;
    }

    if (!SD.rename(
            TemporarySessionPath,
            finalPath))
    {
        SD.remove(TemporarySessionPath);
        Serial.println(
            "StorageService: Session save failed - "
            "temporary record could not be finalized");
        return false;
    }

    return true;
}

StorageService::SessionReadResult
StorageService::ReadMeasurementSession(
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
        return SessionReadResult::OpenFailed;
    }

    const SessionReadResult result =
        ParseMeasurementSession(file, session);

    file.close();

    if (result != SessionReadResult::Success)
    {
        return result;
    }

    if (!session.available ||
        session.sessionId != sessionId ||
        !session.summary.available)
    {
        return SessionReadResult::SessionIdMismatch;
    }

    return SessionReadResult::Success;
}

StorageService::SessionReadResult
StorageService::ParseMeasurementSession(
    fs::File &file,
    StoredWiFiMeasurementSession &session)
{
    session = StoredWiFiMeasurementSession{};

    uint32_t version = 0;
    uint32_t storedChecksum = 0;
    uint32_t calculatedCrc = Crc32Initial;

    bool versionSeen = false;
    bool checksumSeen = false;
    bool idSeen = false;
    bool capturedTimeValidSeen = false;
    bool capturedEpochSeen = false;
    bool capturedLocalSeen = false;
    bool completedAtSeen = false;
    bool scansSeen = false;
    bool networkCountSeen = false;
    bool occupiedChannelsSeen = false;
    bool bestChannelSeen = false;
    bool bestScoreSeen = false;
    bool secondBestScoreSeen = false;
    bool marginSeen = false;
    bool comparableCountSeen = false;
    bool historySamplesSeen = false;
    bool confidenceSeen = false;
    bool uniqueSeen = false;

    bool candidateSeen[
        WiFiMeasurementSummary::CandidateCapacity] = {};
    bool channelSeen[
        WiFiMeasurementSummary::ChannelCapacity] = {};

    while (file.available())
    {
        const String rawLine =
            file.readStringUntil('\n');

        String line = rawLine;
        line.trim();

        if (line.startsWith("checksum_crc32="))
        {
            if (checksumSeen ||
                !ParseHexUnsigned(
                    line.substring(15),
                    storedChecksum))
            {
                return SessionReadResult::ParseFailed;
            }

            checksumSeen = true;
            continue;
        }

        if (checksumSeen)
        {
            if (line.length() != 0)
            {
                return SessionReadResult::ParseFailed;
            }

            continue;
        }

        calculatedCrc = UpdateCrc32(
            calculatedCrc,
            rawLine);

        const uint8_t newline = '\n';
        calculatedCrc = UpdateCrc32(
            calculatedCrc,
            &newline,
            1);

        if (line.length() == 0 ||
            line.startsWith("#"))
        {
            continue;
        }

        const int separator = line.indexOf('=');

        if (separator <= 0)
        {
            return SessionReadResult::ParseFailed;
        }

        const String key =
            line.substring(0, separator);
        const String value =
            line.substring(separator + 1);

        uint32_t parsed = 0;

        if (key == "version")
        {
            if (versionSeen ||
                !ParseUnsigned(value, version))
            {
                return SessionReadResult::ParseFailed;
            }
            versionSeen = true;
        }
        else if (key == "session_id")
        {
            if (idSeen ||
                !ParseUnsigned(value, session.sessionId))
            {
                return SessionReadResult::ParseFailed;
            }
            idSeen = true;
        }
        else if (key == "capture_time_valid")
        {
            if (capturedTimeValidSeen ||
                !ParseBoolean(
                    value,
                    session.capturedTimeValid))
            {
                return SessionReadResult::ParseFailed;
            }
            capturedTimeValidSeen = true;
        }
        else if (key == "captured_epoch")
        {
            if (capturedEpochSeen ||
                !ParseUnsigned(
                    value,
                    session.capturedEpoch))
            {
                return SessionReadResult::ParseFailed;
            }
            capturedEpochSeen = true;
        }
        else if (key == "captured_local")
        {
            if (capturedLocalSeen ||
                value.length() == 0 ||
                value.length() >=
                    StoredWiFiMeasurementSession::
                        CapturedLocalCapacity)
            {
                return SessionReadResult::ParseFailed;
            }

            std::strncpy(
                session.capturedLocal,
                value.c_str(),
                StoredWiFiMeasurementSession::
                    CapturedLocalCapacity - 1);

            session.capturedLocal[
                StoredWiFiMeasurementSession::
                    CapturedLocalCapacity - 1] = '\0';

            capturedLocalSeen = true;
        }
        else if (key == "completed_at_ms")
        {
            if (completedAtSeen ||
                !ParseUnsigned(value, session.completedAtMs))
            {
                return SessionReadResult::ParseFailed;
            }
            completedAtSeen = true;
        }
        else if (key == "completed_scans")
        {
            if (scansSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.completedScanCount =
                static_cast<uint8_t>(parsed);
            scansSeen = true;
        }
        else if (key == "network_count")
        {
            if (networkCountSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.networkCount =
                static_cast<uint8_t>(parsed);
            networkCountSeen = true;
        }
        else if (key == "occupied_channels")
        {
            if (occupiedChannelsSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.occupiedChannelCount =
                static_cast<uint8_t>(parsed);
            occupiedChannelsSeen = true;
        }
        else if (key == "best_channel")
        {
            if (bestChannelSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.bestChannel =
                static_cast<uint8_t>(parsed);
            bestChannelSeen = true;
        }
        else if (key == "best_score")
        {
            if (bestScoreSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 65535)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.bestScore =
                static_cast<uint16_t>(parsed);
            bestScoreSeen = true;
        }
        else if (key == "second_best_score")
        {
            if (secondBestScoreSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 65535)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.secondBestScore =
                static_cast<uint16_t>(parsed);
            secondBestScoreSeen = true;
        }
        else if (key == "score_margin")
        {
            if (marginSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 65535)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.scoreMargin =
                static_cast<uint16_t>(parsed);
            marginSeen = true;
        }
        else if (key == "comparable_count")
        {
            if (comparableCountSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.comparableCount =
                static_cast<uint8_t>(parsed);
            comparableCountSeen = true;
        }
        else if (key == "history_samples")
        {
            if (historySamplesSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > 255)
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.historySampleCount =
                static_cast<uint8_t>(parsed);
            historySamplesSeen = true;
        }
        else if (key == "confidence")
        {
            if (confidenceSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed > static_cast<uint32_t>(
                    WiFiRecommendationConfidence::High))
            {
                return SessionReadResult::ParseFailed;
            }
            session.summary.recommendation.confidence =
                static_cast<WiFiRecommendationConfidence>(parsed);
            confidenceSeen = true;
        }
        else if (key == "unique")
        {
            if (uniqueSeen ||
                !ParseBoolean(
                    value,
                    session.summary.recommendation.unique))
            {
                return SessionReadResult::ParseFailed;
            }
            uniqueSeen = true;
        }
        else if (key.startsWith("candidate_"))
        {
            const long parsedIndex =
                key.substring(10).toInt();

            if (parsedIndex < 0 ||
                parsedIndex >=
                    WiFiMeasurementSummary::CandidateCapacity)
            {
                return SessionReadResult::ParseFailed;
            }

            const uint8_t index =
                static_cast<uint8_t>(parsedIndex);

            if (candidateSeen[index])
            {
                return SessionReadResult::ParseFailed;
            }

            unsigned int channel = 0;
            unsigned int latest = 0;
            unsigned int average = 0;
            unsigned int congestion = 0;
            unsigned int recommended = 0;
            unsigned int comparable = 0;
            char trailing = '\0';

            if (std::sscanf(
                    value.c_str(),
                    "%u,%u,%u,%u,%u,%u%c",
                    &channel,
                    &latest,
                    &average,
                    &congestion,
                    &recommended,
                    &comparable,
                    &trailing) != 6 ||
                channel > 255 ||
                latest > 65535 ||
                average > 65535 ||
                congestion > static_cast<unsigned int>(
                    WiFiCongestionLevel::Excellent) ||
                recommended > 1 ||
                comparable > 1)
            {
                return SessionReadResult::ParseFailed;
            }

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
            candidateSeen[index] = true;
        }
        else if (key.startsWith("channel_"))
        {
            const long parsedIndex =
                key.substring(8).toInt();

            if (parsedIndex < 0 ||
                parsedIndex >=
                    WiFiMeasurementSummary::ChannelCapacity)
            {
                return SessionReadResult::ParseFailed;
            }

            const uint8_t channelIndex =
                static_cast<uint8_t>(parsedIndex);

            if (channelSeen[channelIndex])
            {
                return SessionReadResult::ParseFailed;
            }

            unsigned int channel = 0;
            unsigned int count = 0;
            long strongest = -127;
            long average = -127;
            char trailing = '\0';

            if (std::sscanf(
                    value.c_str(),
                    "%u,%u,%ld,%ld%c",
                    &channel,
                    &count,
                    &strongest,
                    &average,
                    &trailing) != 4 ||
                channel > 255 ||
                count > 255 ||
                strongest < -127 ||
                strongest > 0 ||
                average < -127 ||
                average > 0)
            {
                return SessionReadResult::ParseFailed;
            }

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
            channelSeen[channelIndex] = true;
        }
        // Unknown fields are tolerated to preserve forward
        // compatibility within a supported format version.
    }

    if (!versionSeen)
    {
        return SessionReadResult::IncompleteRecord;
    }

    if (version != 1 &&
        version != 2 &&
        version != CurrentSessionFormatVersion)
    {
        return SessionReadResult::UnsupportedVersion;
    }

    bool complete =
        idSeen &&
        completedAtSeen &&
        scansSeen &&
        networkCountSeen &&
        occupiedChannelsSeen &&
        bestChannelSeen &&
        bestScoreSeen &&
        secondBestScoreSeen &&
        marginSeen &&
        comparableCountSeen &&
        historySamplesSeen &&
        confidenceSeen &&
        uniqueSeen;

    if (version >= 3)
    {
        complete =
            complete &&
            capturedTimeValidSeen &&
            capturedEpochSeen &&
            capturedLocalSeen;

        if (complete &&
            session.capturedTimeValid)
        {
            complete =
                session.capturedEpoch != 0 &&
                session.capturedLocal[0] != '\0' &&
                std::strcmp(
                    session.capturedLocal,
                    "unavailable") != 0;
        }
        else if (complete)
        {
            complete =
                session.capturedEpoch == 0;
        }
    }
    else
    {
        session.capturedTimeValid = false;
        session.capturedEpoch = 0;
        session.capturedLocal[0] = '\0';
    }

    for (uint8_t index = 0;
         index <
             WiFiMeasurementSummary::CandidateCapacity;
         ++index)
    {
        complete = complete && candidateSeen[index];
    }

    for (uint8_t index = 0;
         index <
             WiFiMeasurementSummary::ChannelCapacity;
         ++index)
    {
        complete = complete && channelSeen[index];
    }

    if (!complete)
    {
        return SessionReadResult::IncompleteRecord;
    }

    session.formatVersion =
        static_cast<uint8_t>(version);

    if (version >= 2)
    {
        if (!checksumSeen)
        {
            return SessionReadResult::ChecksumMissing;
        }

        calculatedCrc ^= 0xFFFFFFFFUL;

        if (calculatedCrc != storedChecksum)
        {
            return SessionReadResult::ChecksumMismatch;
        }

        session.integrityVerified = true;
    }
    else
    {
        session.integrityVerified = false;
    }

    session.available = true;
    session.summary.available = true;
    return SessionReadResult::Success;
}

bool StorageService::VerifyTemporarySession(
    const StoredWiFiMeasurementSession &expected)
{
    File file =
        SD.open(TemporarySessionPath, FILE_READ);

    if (!file)
    {
        Serial.println(
            "StorageService: Temporary verification "
            "failed - file could not be reopened");
        return false;
    }

    StoredWiFiMeasurementSession verified{};
    const SessionReadResult result =
        ParseMeasurementSession(file, verified);

    file.close();

    if (result != SessionReadResult::Success)
    {
        Serial.printf(
            "StorageService: Temporary verification "
            "failed - %s\n",
            GetSessionReadResultText(result));
        return false;
    }

    if (verified.sessionId != expected.sessionId ||
        verified.completedAtMs != expected.completedAtMs ||
        verified.capturedTimeValid !=
            expected.capturedTimeValid ||
        verified.capturedEpoch !=
            expected.capturedEpoch ||
        std::strcmp(
            verified.capturedLocal,
            expected.capturedLocal) != 0 ||
        !verified.integrityVerified)
    {
        Serial.println(
            "StorageService: Temporary verification "
            "failed - record identity mismatch");
        return false;
    }

    Serial.printf(
        "StorageService: Session %lu temporary "
        "record verified (CRC32 valid)\n",
        static_cast<unsigned long>(expected.sessionId));

    return true;
}

bool StorageService::WriteSummaryFields(
    fs::File &file,
    const StoredWiFiMeasurementSession &session)
{
    const WiFiMeasurementSummary &summary =
        session.summary;

    uint32_t crc = Crc32Initial;

    if (!WriteCrcLine(
            file,
            crc,
            "version=%u\n",
            CurrentSessionFormatVersion) ||
        !WriteCrcLine(
            file,
            crc,
            "session_id=%lu\n",
            static_cast<unsigned long>(
                session.sessionId)) ||
        !WriteCrcLine(
            file,
            crc,
            "capture_time_valid=%u\n",
            session.capturedTimeValid ? 1 : 0) ||
        !WriteCrcLine(
            file,
            crc,
            "captured_epoch=%lu\n",
            static_cast<unsigned long>(
                session.capturedEpoch)) ||
        !WriteCrcLine(
            file,
            crc,
            "captured_local=%s\n",
            session.capturedTimeValid
                ? session.capturedLocal
                : "unavailable") ||
        !WriteCrcLine(
            file,
            crc,
            "completed_at_ms=%lu\n",
            static_cast<unsigned long>(
                session.completedAtMs)) ||
        !WriteCrcLine(
            file,
            crc,
            "completed_scans=%u\n",
            summary.completedScanCount) ||
        !WriteCrcLine(
            file,
            crc,
            "network_count=%u\n",
            summary.networkCount) ||
        !WriteCrcLine(
            file,
            crc,
            "occupied_channels=%u\n",
            summary.occupiedChannelCount) ||
        !WriteCrcLine(
            file,
            crc,
            "best_channel=%u\n",
            summary.recommendation.bestChannel) ||
        !WriteCrcLine(
            file,
            crc,
            "best_score=%u\n",
            summary.recommendation.bestScore) ||
        !WriteCrcLine(
            file,
            crc,
            "second_best_score=%u\n",
            summary.recommendation.secondBestScore) ||
        !WriteCrcLine(
            file,
            crc,
            "score_margin=%u\n",
            summary.recommendation.scoreMargin) ||
        !WriteCrcLine(
            file,
            crc,
            "comparable_count=%u\n",
            summary.recommendation.comparableCount) ||
        !WriteCrcLine(
            file,
            crc,
            "history_samples=%u\n",
            summary.recommendation.historySampleCount) ||
        !WriteCrcLine(
            file,
            crc,
            "confidence=%u\n",
            static_cast<unsigned int>(
                summary.recommendation.confidence)) ||
        !WriteCrcLine(
            file,
            crc,
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

        if (!WriteCrcLine(
                file,
                crc,
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

        if (!WriteCrcLine(
                file,
                crc,
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

    const uint32_t finalCrc =
        crc ^ 0xFFFFFFFFUL;

    return file.printf(
        "checksum_crc32=%08lX\n",
        static_cast<unsigned long>(finalCrc)) > 0;
}

const char *StorageService::GetSessionReadResultText(
    SessionReadResult result)
{
    switch (result)
    {
        case SessionReadResult::Success:
            return "verified";
        case SessionReadResult::OpenFailed:
            return "file open failed";
        case SessionReadResult::ParseFailed:
            return "malformed record";
        case SessionReadResult::UnsupportedVersion:
            return "unsupported format version";
        case SessionReadResult::ChecksumMissing:
            return "checksum missing";
        case SessionReadResult::ChecksumMismatch:
            return "CRC32 mismatch";
        case SessionReadResult::SessionIdMismatch:
            return "session ID does not match filename";
        case SessionReadResult::IncompleteRecord:
            return "required fields missing";
        default:
            return "unknown read error";
    }
}

uint32_t StorageService::ExtractSessionId(
    const char *path)
{
    uint32_t sessionId = 0;

    if (!TryExtractSessionFileId(
            path,
            ".txt",
            sessionId))
    {
        return 0;
    }

    return sessionId;
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

void StorageService::QuarantineSessionFile(
    uint32_t sessionId,
    SessionReadResult result)
{
    if (result == SessionReadResult::OpenFailed ||
        result == SessionReadResult::UnsupportedVersion)
    {
        return;
    }

    char sourcePath[64];
    BuildSessionPath(
        sessionId,
        sourcePath,
        sizeof(sourcePath));

    char quarantinePath[72];
    std::snprintf(
        quarantinePath,
        sizeof(quarantinePath),
        "%s/session_%06lu.bad",
        SessionsDirectory,
        static_cast<unsigned long>(sessionId));

    if (SD.exists(quarantinePath) &&
        !SD.remove(quarantinePath))
    {
        Serial.printf(
            "StorageService: Warning - session %lu "
            "could not be quarantined; old .bad file "
            "could not be replaced\n",
            static_cast<unsigned long>(sessionId));
        return;
    }

    if (!SD.rename(
            sourcePath,
            quarantinePath))
    {
        Serial.printf(
            "StorageService: Warning - session %lu "
            "could not be quarantined\n",
            static_cast<unsigned long>(sessionId));
        return;
    }

    Serial.printf(
        "StorageService: Session %lu moved to .bad "
        "quarantine\n",
        static_cast<unsigned long>(sessionId));
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
