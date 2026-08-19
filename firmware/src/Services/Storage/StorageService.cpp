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

    constexpr const char *SurveysDirectory =
        "/sentinel/surveys";

    constexpr const char *TemporarySessionPath =
        "/sentinel/sessions/session.tmp";

    constexpr const char *TemporarySiteSurveyPath =
        "/sentinel/surveys/survey.tmp";

    constexpr const char *ValidationPath =
        "/sentinel_test.txt";

    constexpr const char ValidationRecord[] =
        "SentinelOS SD validation 10.15A\n";
        

    constexpr uint64_t BytesPerMegabyte =
        1024ULL * 1024ULL;

    constexpr uint32_t Crc32Initial =
        0xFFFFFFFFUL;

    constexpr const char *ActiveSiteSurveyPath =
        "/sentinel/surveys/active.txt";

    constexpr const char *TemporaryActiveSiteSurveyPath =
        "/sentinel/surveys/active.tmp";

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

    bool ParseRssi(
        const String &value,
        int32_t &result)
    {
        if (value.length() == 0)
        {
            return false;
        }

        char *end = nullptr;
        const long parsed =
            std::strtol(value.c_str(), &end, 10);

        if (end == value.c_str() ||
            *end != '\0' ||
            parsed < -127 ||
            parsed > 0)
        {
            return false;
        }

        result = static_cast<int32_t>(parsed);
        return true;
    }

    int HexNibble(char value)
    {
        if (value >= '0' && value <= '9')
        {
            return value - '0';
        }

        if (value >= 'A' && value <= 'F')
        {
            return value - 'A' + 10;
        }

        if (value >= 'a' && value <= 'f')
        {
            return value - 'a' + 10;
        }

        return -1;
    }

    bool EncodeStorageText(
        const char *source,
        char *destination,
        size_t destinationSize)
    {
        if (source == nullptr ||
            destination == nullptr ||
            destinationSize == 0)
        {
            return false;
        }

        static constexpr char HexDigits[] =
            "0123456789ABCDEF";

        size_t writeIndex = 0;

        for (size_t index = 0;
             source[index] != '\0';
             ++index)
        {
            const uint8_t value =
                static_cast<uint8_t>(source[index]);

            const bool mustEscape =
                value < 0x20 ||
                value > 0x7E ||
                value == '%';

            const size_t required =
                mustEscape ? 3 : 1;

            if (writeIndex + required >=
                destinationSize)
            {
                return false;
            }

            if (mustEscape)
            {
                destination[writeIndex++] = '%';
                destination[writeIndex++] =
                    HexDigits[(value >> 4) & 0x0F];
                destination[writeIndex++] =
                    HexDigits[value & 0x0F];
            }
            else
            {
                destination[writeIndex++] =
                    static_cast<char>(value);
            }
        }

        destination[writeIndex] = '\0';
        return true;
    }

    bool DecodeStorageText(
        const String &source,
        char *destination,
        size_t destinationSize)
    {
        if (destination == nullptr ||
            destinationSize == 0)
        {
            return false;
        }

        size_t writeIndex = 0;

        for (size_t index = 0;
             index < source.length();
             ++index)
        {
            uint8_t value =
                static_cast<uint8_t>(source[index]);

            if (value == '%')
            {
                if (index + 2 >= source.length())
                {
                    return false;
                }

                const int high =
                    HexNibble(source[index + 1]);
                const int low =
                    HexNibble(source[index + 2]);

                if (high < 0 || low < 0)
                {
                    return false;
                }

                value =
                    static_cast<uint8_t>(
                        (high << 4) | low);

                index += 2;
            }

            if (value == 0 ||
                writeIndex + 1 >= destinationSize)
            {
                return false;
            }

            destination[writeIndex++] =
                static_cast<char>(value);
        }

        destination[writeIndex] = '\0';
        return true;
    }

    const char *SecurityToStorageText(
        WiFiSecurity security)
    {
        switch (security)
        {
            case WiFiSecurity::Open:
                return "OPEN";
            case WiFiSecurity::WEP:
                return "WEP";
            case WiFiSecurity::WPA_PSK:
                return "WPA_PSK";
            case WiFiSecurity::WPA2_PSK:
                return "WPA2_PSK";
            case WiFiSecurity::WPA_WPA2_PSK:
                return "WPA_WPA2_PSK";
            case WiFiSecurity::WPA2_Enterprise:
                return "WPA2_ENTERPRISE";
            case WiFiSecurity::WPA3_PSK:
                return "WPA3_PSK";
            case WiFiSecurity::WPA2_WPA3_PSK:
                return "WPA2_WPA3_PSK";
            case WiFiSecurity::WAPI_PSK:
                return "WAPI_PSK";
            case WiFiSecurity::Unknown:
            default:
                return "UNKNOWN";
        }
    }

    bool ParseSecurityText(
        const String &value,
        WiFiSecurity &security)
    {
        if (value == "OPEN")
        {
            security = WiFiSecurity::Open;
        }
        else if (value == "WEP")
        {
            security = WiFiSecurity::WEP;
        }
        else if (value == "WPA_PSK")
        {
            security = WiFiSecurity::WPA_PSK;
        }
        else if (value == "WPA2_PSK")
        {
            security = WiFiSecurity::WPA2_PSK;
        }
        else if (value == "WPA_WPA2_PSK")
        {
            security = WiFiSecurity::WPA_WPA2_PSK;
        }
        else if (value == "WPA2_ENTERPRISE")
        {
            security = WiFiSecurity::WPA2_Enterprise;
        }
        else if (value == "WPA3_PSK")
        {
            security = WiFiSecurity::WPA3_PSK;
        }
        else if (value == "WPA2_WPA3_PSK")
        {
            security = WiFiSecurity::WPA2_WPA3_PSK;
        }
        else if (value == "WAPI_PSK")
        {
            security = WiFiSecurity::WAPI_PSK;
        }
        else if (value == "UNKNOWN")
        {
            security = WiFiSecurity::Unknown;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool ParseBssid(
        const String &value,
        uint8_t *bssid)
    {
        if (bssid == nullptr)
        {
            return false;
        }

        unsigned int bytes[
            WiFiNetworkInfo::BssidLength] = {};

        char trailing = '\0';

        if (std::sscanf(
                value.c_str(),
                "%2x:%2x:%2x:%2x:%2x:%2x%c",
                &bytes[0],
                &bytes[1],
                &bytes[2],
                &bytes[3],
                &bytes[4],
                &bytes[5],
                &trailing) !=
            WiFiNetworkInfo::BssidLength)
        {
            return false;
        }

        for (uint8_t index = 0;
             index < WiFiNetworkInfo::BssidLength;
             ++index)
        {
            if (bytes[index] > 0xFF)
            {
                return false;
            }

            bssid[index] =
                static_cast<uint8_t>(bytes[index]);
        }

        return true;
    }

    WiFiSignalQuality ClassifyStoredSignal(
        int32_t rssi)
    {
        if (rssi >= -55)
        {
            return WiFiSignalQuality::Excellent;
        }

        if (rssi >= -67)
        {
            return WiFiSignalQuality::Good;
        }

        if (rssi >= -75)
        {
            return WiFiSignalQuality::Fair;
        }

        return WiFiSignalQuality::Poor;
    }

    constexpr uint16_t NetworkFieldSsid =
        1U << 0;
    constexpr uint16_t NetworkFieldBssid =
        1U << 1;
    constexpr uint16_t NetworkFieldChannel =
        1U << 2;
    constexpr uint16_t NetworkFieldSecurity =
        1U << 3;
    constexpr uint16_t NetworkFieldHidden =
        1U << 4;
    constexpr uint16_t NetworkFieldSeen =
        1U << 5;
    constexpr uint16_t NetworkFieldAverageRssi =
        1U << 6;
    constexpr uint16_t NetworkFieldMinimumRssi =
        1U << 7;
    constexpr uint16_t NetworkFieldMaximumRssi =
        1U << 8;

    constexpr uint16_t NetworkFieldAll =
        NetworkFieldSsid |
        NetworkFieldBssid |
        NetworkFieldChannel |
        NetworkFieldSecurity |
        NetworkFieldHidden |
        NetworkFieldSeen |
        NetworkFieldAverageRssi |
        NetworkFieldMinimumRssi |
        NetworkFieldMaximumRssi;

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

// Full v4 sessions include up to 128 detailed AP records. Keep these
// scratch objects in static storage instead of the Arduino loop-task
// stack, which is too small for multi-kilobyte session objects.
static StoredWiFiMeasurementSession
    storageSessionScratch{};

static StoredWiFiMeasurementSession
    storageVerificationScratch{};

// Version 4 session records contain the full AP inventory and are now
// several kilobytes. Reuse a static default object whenever a session must
// be cleared so brace-initialized temporaries are not materialized on the
// Arduino loop-task stack during startup, parsing, or verification.
static const StoredWiFiMeasurementSession
    emptyStoredSession{};

bool StorageService::available = false;
uint8_t StorageService::detectedCardType = CARD_NONE;
uint64_t StorageService::cardCapacityBytes = 0;
uint64_t StorageService::filesystemTotalBytes = 0;
uint64_t StorageService::filesystemUsedBytes = 0;
StorageValidationResult StorageService::validationResult =
    StorageValidationResult::NotRun;

bool StorageService::externalReadOnlyAccessActive = false;

StoredWiFiMeasurementSessionIndex
    StorageService::savedSessionIndex[
        StorageService::MaxSavedSessions] = {};

StoredWiFiMeasurementSession
    StorageService::loadedSession{};

uint8_t StorageService::loadedSessionIndex =
    StorageService::InvalidLoadedSessionIndex;

uint8_t StorageService::savedSessionCount = 0;
uint32_t StorageService::nextSessionId = 1;
uint32_t StorageService::nextSiteSurveyId = 1;

void StorageService::Begin()
{
    available = false;
    detectedCardType = CARD_NONE;
    cardCapacityBytes = 0;
    filesystemTotalBytes = 0;
    filesystemUsedBytes = 0;
    validationResult = StorageValidationResult::NotRun;
    externalReadOnlyAccessActive = false;
    savedSessionCount = 0;
    nextSessionId = 1;
    nextSiteSurveyId = 1;
    loadedSession = emptyStoredSession;
    loadedSessionIndex = InvalidLoadedSessionIndex;

    for (uint8_t index = 0;
         index < MaxSavedSessions;
         ++index)
    {
        savedSessionIndex[index] =
            StoredWiFiMeasurementSessionIndex{};
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
    LoadSiteSurveySequence();

    Serial.println(
        "StorageService: Startup validation skipped; "
        "run-once validation already completed");
}

bool StorageService::RunValidation()
{
    if (externalReadOnlyAccessActive)
    {
        Serial.println(
            "StorageService: Validation blocked while "
            "USB Storage Mode is active");
        return false;
    }

    return RunReadWriteValidation();
}

bool StorageService::IsAvailable()
{
    return available;
}

void StorageService::SetExternalReadOnlyAccessActive(
    bool active)
{
    externalReadOnlyAccessActive = active;

    Serial.printf(
        "StorageService: External read-only access %s\n",
        active ? "ACTIVE" : "released");
}

bool StorageService::IsExternalReadOnlyAccessActive()
{
    return externalReadOnlyAccessActive;
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
    if (externalReadOnlyAccessActive)
    {
        Serial.println(
            "StorageService: Session save blocked while "
            "USB Storage Mode is active");
        return false;
    }

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

    StoredWiFiMeasurementSession &session =
        storageSessionScratch;

    session = emptyStoredSession;
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
            savedSessionIndex[
                MaxSavedSessions - 1]
                .sessionId;
    }

    InsertSavedSessionIndex(session);

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
        "StorageService: Session %lu includes "
        "%u unique BSSID%s across %u completed scans\n",
        static_cast<unsigned long>(
            session.sessionId),
        session.summary.observedNetworkCount,
        session.summary.observedNetworkCount == 1
            ? ""
            : "s",
        session.summary.completedScanCount);

    Serial.printf(
        "StorageService: Session %lu saved, "
        "%u/%u retained\n",
        static_cast<unsigned long>(
            session.sessionId),
        savedSessionCount,
        MaxSavedSessions);

    return true;
}

bool StorageService::CreateSiteSurveyRecord(
    const char *name,
    uint32_t createdEpoch,
    uint32_t &surveyId)
{
    surveyId = 0;

    if (externalReadOnlyAccessActive)
    {
        Serial.println(
            "StorageService: Site Survey creation blocked "
            "while USB Storage Mode is active");
        return false;
    }

    if (!available ||
        name == nullptr ||
        name[0] == '\0')
    {
        return false;
    }

    if (!EnsureDirectories())
    {
        return false;
    }

    StoredSiteSurvey survey{};

    survey.available = true;
    survey.formatVersion =
        CurrentSiteSurveyFormatVersion;
    survey.surveyId =
        nextSiteSurveyId;
    survey.createdEpoch =
        createdEpoch;

    std::strncpy(
        survey.name,
        name,
        StoredSiteSurvey::NameCapacity - 1);

    survey.name[
        StoredSiteSurvey::NameCapacity - 1] = '\0';

    if (!WriteSiteSurveyRecord(survey))
    {
        Serial.printf(
            "StorageService: Site Survey %lu "
            "creation failed\n",
            static_cast<unsigned long>(
                survey.surveyId));

        return false;
    }

    surveyId = survey.surveyId;

    ++nextSiteSurveyId;

    if (nextSiteSurveyId == 0)
    {
        nextSiteSurveyId = 1;
    }

    filesystemUsedBytes =
        SD.usedBytes();

    Serial.printf(
        "StorageService: Site Survey %lu created: %s\n",
        static_cast<unsigned long>(surveyId),
        survey.name);

    return true;
}

uint8_t StorageService::GetSavedSessionCount()
{
    return savedSessionCount;
}

uint32_t StorageService::GetNextSiteSurveyId()
{
    return nextSiteSurveyId;
}

const StoredWiFiMeasurementSessionIndex *
StorageService::GetSavedSessionIndex(uint8_t index)
{
    if (index >= savedSessionCount)
    {
        return nullptr;
    }

    return &savedSessionIndex[index];
}

const StoredWiFiMeasurementSession *
StorageService::GetSavedSession(uint8_t index)
{
    if (index >= savedSessionCount)
    {
        return nullptr;
    }

    const uint32_t sessionId =
        savedSessionIndex[index].sessionId;

    if (loadedSessionIndex == index &&
        loadedSession.available &&
        loadedSession.sessionId == sessionId)
    {
        return &loadedSession;
    }

    StoredWiFiMeasurementSession &session =
        storageSessionScratch;

    session = emptyStoredSession;

    const SessionReadResult result =
        ReadMeasurementSession(
            sessionId,
            session);

    if (result != SessionReadResult::Success)
    {
        Serial.printf(
            "StorageService: Session %lu skipped on open - %s\n",
            static_cast<unsigned long>(sessionId),
            GetSessionReadResultText(result));

        QuarantineSessionFile(
            sessionId,
            result);

        RemoveIndexedSessionAt(index);
        return nullptr;
    }

    if (session.integrityVerified)
    {
        Serial.printf(
            "StorageService: Session %lu verified on demand "
            "(format v%u, CRC32 valid)\n",
            static_cast<unsigned long>(
                session.sessionId),
            session.formatVersion);
    }
    else
    {
        Serial.printf(
            "StorageService: Session %lu loaded on demand as "
            "legacy format v%u without checksum\n",
            static_cast<unsigned long>(
                session.sessionId),
            session.formatVersion);
    }

    loadedSession = session;
    loadedSessionIndex = index;
    UpdateIndexMetadata(
        index,
        loadedSession);

    return &loadedSession;
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

    if (!SD.exists(SurveysDirectory) &&
        !SD.mkdir(SurveysDirectory))
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

bool StorageService::SaveActiveSiteSurvey(
    uint32_t surveyId,
    const char *name,
    uint32_t createdEpoch)
{
    if (externalReadOnlyAccessActive)
    {
        Serial.println(
            "StorageService: Active Site Survey save blocked "
            "while USB Storage Mode is active");
        return false;
    }

    if (!available ||
        surveyId == 0 ||
        name == nullptr ||
        name[0] == '\0')
    {
        Serial.println(
            "StorageService: Active Site Survey save skipped - "
            "invalid survey or storage unavailable");
        return false;
    }

    if (!EnsureDirectories())
    {
        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "storage directories unavailable");
        return false;
    }

    char encodedName[
        StoredActiveSiteSurvey::NameCapacity * 3] = {};

    if (!EncodeStorageText(
            name,
            encodedName,
            sizeof(encodedName)))
    {
        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "name could not be encoded");
        return false;
    }

    if (SD.exists(TemporaryActiveSiteSurveyPath) &&
        !SD.remove(TemporaryActiveSiteSurveyPath))
    {
        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "stale temporary file could not be removed");
        return false;
    }

    File file =
        SD.open(
            TemporaryActiveSiteSurveyPath,
            FILE_WRITE);

    if (!file)
    {
        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "temporary file could not be created");
        return false;
    }

    uint32_t crc = Crc32Initial;

    const bool written =
        WriteCrcLine(
            file,
            crc,
            "version=%u\n",
            CurrentActiveSiteSurveyFormatVersion) &&
        WriteCrcLine(
            file,
            crc,
            "site_survey_id=%lu\n",
            static_cast<unsigned long>(surveyId)) &&
        WriteCrcLine(
            file,
            crc,
            "created_epoch=%lu\n",
            static_cast<unsigned long>(createdEpoch)) &&
        WriteCrcLine(
            file,
            crc,
            "name=%s\n",
            encodedName);

    if (written)
    {
        const uint32_t finalCrc =
            crc ^ 0xFFFFFFFFUL;

        if (file.printf(
                "checksum_crc32=%08lX\n",
                static_cast<unsigned long>(
                    finalCrc)) <= 0)
        {
            file.close();
            SD.remove(
                TemporaryActiveSiteSurveyPath);

            Serial.println(
                "StorageService: Active Site Survey save "
                "failed - checksum could not be written");
            return false;
        }
    }

    file.flush();
    file.close();

    if (!written)
    {
        SD.remove(
            TemporaryActiveSiteSurveyPath);

        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "record was not written completely");
        return false;
    }

    if (SD.exists(ActiveSiteSurveyPath) &&
        !SD.remove(ActiveSiteSurveyPath))
    {
        SD.remove(
            TemporaryActiveSiteSurveyPath);

        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "existing active record could not be replaced");
        return false;
    }

    if (!SD.rename(
            TemporaryActiveSiteSurveyPath,
            ActiveSiteSurveyPath))
    {
        SD.remove(
            TemporaryActiveSiteSurveyPath);

        Serial.println(
            "StorageService: Active Site Survey save failed - "
            "temporary record could not be finalized");
        return false;
    }

    filesystemUsedBytes =
        SD.usedBytes();

    Serial.printf(
        "StorageService: Active Site Survey %lu saved\n",
        static_cast<unsigned long>(surveyId));

    return true;
}

bool StorageService::LoadActiveSiteSurvey(
    StoredActiveSiteSurvey &survey)
{
    survey = StoredActiveSiteSurvey{};

    if (!available ||
        !SD.exists(ActiveSiteSurveyPath))
    {
        return false;
    }

    File file =
        SD.open(
            ActiveSiteSurveyPath,
            FILE_READ);

    if (!file)
    {
        Serial.println(
            "StorageService: Active Site Survey could not "
            "be opened");
        return false;
    }

    uint32_t version = 0;
    uint32_t surveyId = 0;
    uint32_t createdEpoch = 0;
    uint32_t storedChecksum = 0;
    uint32_t calculatedCrc = Crc32Initial;

    char decodedName[
        StoredActiveSiteSurvey::NameCapacity] = {};

    bool versionSeen = false;
    bool surveyIdSeen = false;
    bool createdEpochSeen = false;
    bool nameSeen = false;
    bool checksumSeen = false;

    bool valid = true;

    while (file.available() && valid)
    {
        const String rawLine =
            file.readStringUntil('\n');

        String line = rawLine;
        line.trim();

        if (line.startsWith(
                "checksum_crc32="))
        {
            if (checksumSeen ||
                !ParseHexUnsigned(
                    line.substring(15),
                    storedChecksum))
            {
                valid = false;
                break;
            }

            checksumSeen = true;
            continue;
        }

        if (checksumSeen)
        {
            if (line.length() != 0)
            {
                valid = false;
            }

            continue;
        }

        calculatedCrc =
            UpdateCrc32(
                calculatedCrc,
                rawLine);

        const uint8_t newline = '\n';

        calculatedCrc =
            UpdateCrc32(
                calculatedCrc,
                &newline,
                1);

        if (line.length() == 0)
        {
            continue;
        }

        const int separator =
            line.indexOf('=');

        if (separator <= 0)
        {
            valid = false;
            break;
        }

        const String key =
            line.substring(
                0,
                separator);

        const String value =
            line.substring(
                separator + 1);

        if (key == "version")
        {
            if (versionSeen ||
                !ParseUnsigned(
                    value,
                    version))
            {
                valid = false;
            }

            versionSeen = true;
        }
        else if (key == "site_survey_id")
        {
            if (surveyIdSeen ||
                !ParseUnsigned(
                    value,
                    surveyId))
            {
                valid = false;
            }

            surveyIdSeen = true;
        }
        else if (key == "created_epoch")
        {
            if (createdEpochSeen ||
                !ParseUnsigned(
                    value,
                    createdEpoch))
            {
                valid = false;
            }

            createdEpochSeen = true;
        }
        else if (key == "name")
        {
            if (nameSeen ||
                !DecodeStorageText(
                    value,
                    decodedName,
                    sizeof(decodedName)))
            {
                valid = false;
            }

            nameSeen = true;
        }
        else
        {
            valid = false;
        }
    }

    file.close();

    const uint32_t finalCrc =
        calculatedCrc ^ 0xFFFFFFFFUL;

    if (!valid ||
        !versionSeen ||
        !surveyIdSeen ||
        !createdEpochSeen ||
        !nameSeen ||
        !checksumSeen ||
        version !=
            CurrentActiveSiteSurveyFormatVersion ||
        surveyId == 0 ||
        decodedName[0] == '\0' ||
        storedChecksum != finalCrc)
    {
        Serial.println(
            "StorageService: Active Site Survey record "
            "is invalid");
        return false;
    }

    survey.available = true;
    survey.formatVersion =
        static_cast<uint8_t>(version);
    survey.surveyId = surveyId;
    survey.createdEpoch = createdEpoch;

    std::strncpy(
        survey.name,
        decodedName,
        sizeof(survey.name) - 1);

    survey.name[
        sizeof(survey.name) - 1] = '\0';

    Serial.printf(
        "StorageService: Active Site Survey %lu loaded "
        "(%s)\n",
        static_cast<unsigned long>(
            survey.surveyId),
        survey.name);

    return true;
}

bool StorageService::ClearActiveSiteSurvey()
{
    if (externalReadOnlyAccessActive)
    {
        Serial.println(
            "StorageService: Active Site Survey clear blocked "
            "while USB Storage Mode is active");
        return false;
    }

    if (!available)
    {
        return false;
    }

    if (!SD.exists(ActiveSiteSurveyPath))
    {
        return true;
    }

    if (!SD.remove(ActiveSiteSurveyPath))
    {
        Serial.println(
            "StorageService: Active Site Survey record "
            "could not be removed");
        return false;
    }

    filesystemUsedBytes =
        SD.usedBytes();

    Serial.println(
        "StorageService: Active Site Survey cleared");

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
                // Quarantined IDs still advance the sequence so a
                // damaged record is never silently reused.
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
            "StorageService: Warning - session enumeration "
            "limited to %u files\n",
            EnumeratedSessionCapacity);
    }

    SortDescending(
        sessionIds,
        enumeratedCount);

    const uint8_t retainedCount =
        enumeratedCount < MaxSavedSessions
            ? enumeratedCount
            : MaxSavedSessions;

    for (uint8_t index = 0;
         index < retainedCount;
         ++index)
    {
        savedSessionIndex[index] =
            StoredWiFiMeasurementSessionIndex{};
        savedSessionIndex[index].available = true;
        savedSessionIndex[index].sessionId =
            sessionIds[index];
    }

    savedSessionCount = retainedCount;

    // Retention is based on the monotonic session ID. Detailed
    // records are intentionally not parsed here; full CRC validation
    // is deferred until a user opens a session from History.
    for (uint8_t index = retainedCount;
         index < enumeratedCount;
         ++index)
    {
        RemoveSessionFile(sessionIds[index]);
    }

    nextSessionId = maximumSessionId + 1;

    if (nextSessionId == 0)
    {
        nextSessionId = 1;
    }

    Serial.printf(
        "StorageService: Indexed %u saved measurement "
        "session%s; full verification deferred until opened; "
        "next ID %lu\n",
        savedSessionCount,
        savedSessionCount == 1 ? "" : "s",
        static_cast<unsigned long>(nextSessionId));
}

void StorageService::LoadSiteSurveySequence()
{
    uint32_t maximumSurveyId = 0;

    File directory =
        SD.open(SurveysDirectory);

    if (!directory ||
        !directory.isDirectory())
    {
        Serial.println(
            "StorageService: Survey directory unavailable");
        return;
    }

    File entry =
        directory.openNextFile();

    while (entry)
    {
        if (!entry.isDirectory())
        {
            const char *path =
                entry.name();

            const char *name =
                std::strrchr(path, '/');

            name =
                name == nullptr
                    ? path
                    : name + 1;

            constexpr const char Prefix[] =
                "survey_";

            constexpr size_t PrefixLength =
                sizeof(Prefix) - 1;

            if (std::strncmp(
                    name,
                    Prefix,
                    PrefixLength) == 0)
            {
                const char *numberStart =
                    name + PrefixLength;

                char *numberEnd = nullptr;

                const unsigned long parsed =
                    std::strtoul(
                        numberStart,
                        &numberEnd,
                        10);

                if (numberEnd != numberStart &&
                    parsed != 0 &&
                    std::strcmp(
                        numberEnd,
                        ".txt") == 0 &&
                    parsed > maximumSurveyId)
                {
                    maximumSurveyId =
                        static_cast<uint32_t>(
                            parsed);
                }
            }
        }

        entry.close();
        entry =
            directory.openNextFile();
    }

    directory.close();

    nextSiteSurveyId =
        maximumSurveyId + 1;

    if (nextSiteSurveyId == 0)
    {
        nextSiteSurveyId = 1;
    }

    Serial.printf(
        "StorageService: Next Site Survey ID %lu\n",
        static_cast<unsigned long>(
            nextSiteSurveyId));
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

bool StorageService::WriteSiteSurveyRecord(
    const StoredSiteSurvey &survey)
{
    if (SD.exists(TemporarySiteSurveyPath) &&
        !SD.remove(TemporarySiteSurveyPath))
    {
        Serial.println(
            "StorageService: Site Survey save failed - "
            "stale temporary file could not be removed");
        return false;
    }

    File file =
        SD.open(
            TemporarySiteSurveyPath,
            FILE_WRITE);

    if (!file)
    {
        return false;
    }

    uint32_t crc =
        Crc32Initial;

    char encodedName[
        StoredSiteSurvey::NameCapacity * 3] = {};

    const bool encoded =
        EncodeStorageText(
            survey.name,
            encodedName,
            sizeof(encodedName));

    const bool written =
        encoded &&
        WriteCrcLine(
            file,
            crc,
            "version=%u\n",
            CurrentSiteSurveyFormatVersion) &&
        WriteCrcLine(
            file,
            crc,
            "survey_id=%lu\n",
            static_cast<unsigned long>(
                survey.surveyId)) &&
        WriteCrcLine(
            file,
            crc,
            "created_epoch=%lu\n",
            static_cast<unsigned long>(
                survey.createdEpoch)) &&
        WriteCrcLine(
            file,
            crc,
            "name=%s\n",
            encodedName);

    bool checksumWritten = false;

    if (written)
    {
        const uint32_t finalCrc =
            crc ^ 0xFFFFFFFFUL;

        checksumWritten =
            file.printf(
                "checksum_crc32=%08lX\n",
                static_cast<unsigned long>(
                    finalCrc)) > 0;
    }

    file.flush();
    file.close();

    if (!written ||
        !checksumWritten)
    {
        SD.remove(
            TemporarySiteSurveyPath);
        return false;
    }

    char finalPath[64];

    BuildSiteSurveyPath(
        survey.surveyId,
        finalPath,
        sizeof(finalPath));

    // A survey ID must never overwrite an existing survey.
    if (SD.exists(finalPath))
    {
        SD.remove(
            TemporarySiteSurveyPath);

        Serial.println(
            "StorageService: Site Survey save failed - "
            "destination already exists");

        return false;
    }

    if (!SD.rename(
            TemporarySiteSurveyPath,
            finalPath))
    {
        SD.remove(
            TemporarySiteSurveyPath);

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
    session = emptyStoredSession;

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
    bool observedNetworkCountSeen = false;
    bool occupiedChannelsSeen = false;
    bool bestChannelSeen = false;
    bool bestScoreSeen = false;
    bool secondBestScoreSeen = false;
    bool marginSeen = false;
    bool comparableCountSeen = false;
    bool historySamplesSeen = false;
    bool confidenceSeen = false;
    bool uniqueSeen = false;
    bool surveyPointSeen = false;
    bool siteSurveyIdSeen = false;
    bool siteSurveyNameSeen = false;

    bool candidateSeen[
        WiFiMeasurementSummary::CandidateCapacity] = {};
    bool channelSeen[
        WiFiMeasurementSummary::ChannelCapacity] = {};

    uint16_t networkFieldMask[
        WiFiMeasurementSummary::NetworkCapacity] = {};

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
        else if (key == "survey_point")
        {
            if (surveyPointSeen ||
                !DecodeStorageText(
                    value,
                    session.summary.surveyPoint,
                    WiFiMeasurementSummary::
                        SurveyPointCapacity))
            {
                return SessionReadResult::ParseFailed;
            }

            surveyPointSeen = true;
        }

        else if (key == "site_survey_id")
        {
            if (siteSurveyIdSeen ||
                !ParseUnsigned(
                    value,
                    session.summary.siteSurveyId))
            {
                return SessionReadResult::ParseFailed;
            }

            siteSurveyIdSeen = true;
        }
        else if (key == "site_survey_name")
        {
            if (siteSurveyNameSeen ||
                !DecodeStorageText(
                    value,
                    session.summary.siteSurveyName,
                    WiFiMeasurementSummary::
                        SiteSurveyNameCapacity))
            {
                return SessionReadResult::ParseFailed;
            }

            siteSurveyNameSeen = true;
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
        else if (key == "observed_network_count")
        {
            if (observedNetworkCountSeen ||
                !ParseUnsigned(value, parsed) ||
                parsed >
                    WiFiMeasurementSummary::NetworkCapacity)
            {
                return SessionReadResult::ParseFailed;
            }

            session.summary.observedNetworkCount =
                static_cast<uint8_t>(parsed);

            observedNetworkCountSeen = true;
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
        else if (key.startsWith("network_"))
        {
            const int fieldSeparator =
                key.indexOf('_', 8);

            if (fieldSeparator <= 8)
            {
                return SessionReadResult::ParseFailed;
            }

            const String indexText =
                key.substring(8, fieldSeparator);

            uint32_t networkIndex = 0;

            if (!ParseUnsigned(
                    indexText,
                    networkIndex) ||
                networkIndex >=
                    WiFiMeasurementSummary::NetworkCapacity)
            {
                return SessionReadResult::ParseFailed;
            }

            const String field =
                key.substring(fieldSeparator + 1);

            WiFiMeasuredNetwork &network =
                session.summary.networks[
                    networkIndex];

            uint16_t fieldBit = 0;

            if (field == "ssid")
            {
                fieldBit = NetworkFieldSsid;

                if (!DecodeStorageText(
                        value,
                        network.ssid,
                        WiFiMeasuredNetwork::SsidCapacity))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "bssid")
            {
                fieldBit = NetworkFieldBssid;

                if (!ParseBssid(
                        value,
                        network.bssid))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "channel")
            {
                fieldBit = NetworkFieldChannel;

                if (!ParseUnsigned(value, parsed) ||
                    parsed > 255)
                {
                    return SessionReadResult::ParseFailed;
                }

                network.channel =
                    static_cast<uint8_t>(parsed);
            }
            else if (field == "security")
            {
                fieldBit = NetworkFieldSecurity;

                if (!ParseSecurityText(
                        value,
                        network.security))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "hidden")
            {
                fieldBit = NetworkFieldHidden;

                if (!ParseBoolean(
                        value,
                        network.hidden))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "seen")
            {
                fieldBit = NetworkFieldSeen;

                if (!ParseUnsigned(value, parsed) ||
                    parsed == 0 ||
                    parsed > 255)
                {
                    return SessionReadResult::ParseFailed;
                }

                network.seenCount =
                    static_cast<uint8_t>(parsed);
            }
            else if (field == "rssi_avg")
            {
                fieldBit = NetworkFieldAverageRssi;

                if (!ParseRssi(
                        value,
                        network.averageRssi))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "rssi_min")
            {
                fieldBit = NetworkFieldMinimumRssi;

                if (!ParseRssi(
                        value,
                        network.minimumRssi))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else if (field == "rssi_max")
            {
                fieldBit = NetworkFieldMaximumRssi;

                if (!ParseRssi(
                        value,
                        network.maximumRssi))
                {
                    return SessionReadResult::ParseFailed;
                }
            }
            else
            {
                return SessionReadResult::ParseFailed;
            }

            if ((networkFieldMask[networkIndex] &
                 fieldBit) != 0)
            {
                return SessionReadResult::ParseFailed;
            }

            networkFieldMask[networkIndex] |=
                fieldBit;
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
        version != 3 &&
        version != 4 &&
        version != 5 &&
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

    if (version >= 4)
    {
        complete =
            complete &&
            observedNetworkCountSeen;

        if (complete)
        {
            for (uint8_t index = 0;
                 index <
                     WiFiMeasurementSummary::NetworkCapacity;
                 ++index)
            {
                const bool expected =
                    index <
                    session.summary.observedNetworkCount;

                if (expected)
                {
                    complete =
                        complete &&
                        networkFieldMask[index] ==
                            NetworkFieldAll;

                    WiFiMeasuredNetwork &network =
                        session.summary.networks[index];

                    if (complete)
                    {
                        complete =
                            network.seenCount > 0 &&
                            network.seenCount <=
                                session.summary.
                                    completedScanCount &&
                            network.minimumRssi <=
                                network.averageRssi &&
                            network.averageRssi <=
                                network.maximumRssi;
                    }

                    network.signalQuality =
                        ClassifyStoredSignal(
                            network.averageRssi);
                }
                else
                {
                    complete =
                        complete &&
                        networkFieldMask[index] == 0;
                }
            }
        }
    }
    else
    {
        session.summary.observedNetworkCount = 0;
    }

    if (version >= 5)
    {
        complete =
            complete &&
            surveyPointSeen;
    }
    else
    {
        session.summary.surveyPoint[0] = '\0';
    }

    if (version >= 6)
    {
        complete =
            complete &&
            siteSurveyIdSeen &&
            siteSurveyNameSeen;
    }
    else
    {
        session.summary.siteSurveyId = 0;
        session.summary.siteSurveyName[0] = '\0';
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

    StoredWiFiMeasurementSession &verified =
        storageVerificationScratch;

    verified = emptyStoredSession;

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
        std::strcmp(
            verified.summary.surveyPoint,
            expected.summary.surveyPoint) != 0 ||
        verified.summary.siteSurveyId !=
            expected.summary.siteSurveyId ||
        std::strcmp(
            verified.summary.siteSurveyName,
            expected.summary.siteSurveyName) != 0 ||
        verified.summary.observedNetworkCount !=
            expected.summary.observedNetworkCount ||
        !verified.integrityVerified)
        
    {
        Serial.println(
            "StorageService: Temporary verification "
            "failed - record identity mismatch");
        return false;
    }

    for (uint8_t index = 0;
         index < expected.summary.observedNetworkCount;
         ++index)
    {
        const WiFiMeasuredNetwork &expectedNetwork =
            expected.summary.networks[index];

        const WiFiMeasuredNetwork &verifiedNetwork =
            verified.summary.networks[index];

        if (std::strcmp(
                expectedNetwork.ssid,
                verifiedNetwork.ssid) != 0 ||
            std::memcmp(
                expectedNetwork.bssid,
                verifiedNetwork.bssid,
                WiFiMeasuredNetwork::BssidLength) != 0 ||
            expectedNetwork.channel !=
                verifiedNetwork.channel ||
            expectedNetwork.security !=
                verifiedNetwork.security ||
            expectedNetwork.hidden !=
                verifiedNetwork.hidden ||
            expectedNetwork.seenCount !=
                verifiedNetwork.seenCount ||
            expectedNetwork.averageRssi !=
                verifiedNetwork.averageRssi ||
            expectedNetwork.minimumRssi !=
                verifiedNetwork.minimumRssi ||
            expectedNetwork.maximumRssi !=
                verifiedNetwork.maximumRssi)
        {
            Serial.printf(
                "StorageService: Temporary verification "
                "failed - network %u round-trip mismatch\n",
                index);
            return false;
        }
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

    char encodedSurveyPoint[
        WiFiMeasurementSummary::SurveyPointCapacity * 3] = {};

    if (!EncodeStorageText(
            summary.surveyPoint,
            encodedSurveyPoint,
            sizeof(encodedSurveyPoint)))
    {
        return false;
    }

    char encodedSiteSurveyName[
        WiFiMeasurementSummary::
            SiteSurveyNameCapacity * 3] = {};

    if (!EncodeStorageText(
            summary.siteSurveyName,
            encodedSiteSurveyName,
            sizeof(encodedSiteSurveyName)))
    {
        return false;
    }

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
            "survey_point=%s\n",
            encodedSurveyPoint) ||
        !WriteCrcLine(
            file,
            crc,
            "site_survey_id=%lu\n",
            static_cast<unsigned long>(
                summary.siteSurveyId)) ||
        !WriteCrcLine(
            file,
            crc,
            "site_survey_name=%s\n",
            encodedSiteSurveyName) ||
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
            "observed_network_count=%u\n",
            summary.observedNetworkCount) ||
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

    for (uint8_t index = 0;
         index < summary.observedNetworkCount &&
         index < WiFiMeasurementSummary::NetworkCapacity;
         ++index)
    {
        const WiFiMeasuredNetwork &network =
            summary.networks[index];

        char encodedSsid[
            WiFiMeasuredNetwork::SsidCapacity * 3] = {};

        if (!EncodeStorageText(
                network.ssid,
                encodedSsid,
                sizeof(encodedSsid)))
        {
            return false;
        }

        if (!WriteCrcLine(
                file,
                crc,
                "network_%u_ssid=%s\n",
                index,
                encodedSsid) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_bssid="
                "%02X:%02X:%02X:%02X:%02X:%02X\n",
                index,
                network.bssid[0],
                network.bssid[1],
                network.bssid[2],
                network.bssid[3],
                network.bssid[4],
                network.bssid[5]) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_channel=%u\n",
                index,
                network.channel) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_security=%s\n",
                index,
                SecurityToStorageText(
                    network.security)) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_hidden=%u\n",
                index,
                network.hidden ? 1 : 0) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_seen=%u\n",
                index,
                network.seenCount) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_rssi_avg=%ld\n",
                index,
                static_cast<long>(
                    network.averageRssi)) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_rssi_min=%ld\n",
                index,
                static_cast<long>(
                    network.minimumRssi)) ||
            !WriteCrcLine(
                file,
                crc,
                "network_%u_rssi_max=%ld\n",
                index,
                static_cast<long>(
                    network.maximumRssi)))
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

void StorageService::BuildSiteSurveyPath(
    uint32_t surveyId,
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
        "%s/survey_%06lu.txt",
        SurveysDirectory,
        static_cast<unsigned long>(surveyId));
}

void StorageService::InsertSavedSessionIndex(
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
        savedSessionIndex[index] =
            savedSessionIndex[index - 1];
    }

    savedSessionIndex[0] =
        StoredWiFiMeasurementSessionIndex{};
    savedSessionIndex[0].available = true;
    savedSessionIndex[0].sessionId =
        session.sessionId;

    UpdateIndexMetadata(
        0,
        session);

    if (savedSessionCount < MaxSavedSessions)
    {
        ++savedSessionCount;
    }

    InvalidateLoadedSession();
}

void StorageService::RemoveIndexedSessionAt(
    uint8_t index)
{
    if (index >= savedSessionCount)
    {
        return;
    }

    for (uint8_t current = index;
         current + 1 < savedSessionCount;
         ++current)
    {
        savedSessionIndex[current] =
            savedSessionIndex[current + 1];
    }

    if (savedSessionCount > 0)
    {
        --savedSessionCount;
        savedSessionIndex[savedSessionCount] =
            StoredWiFiMeasurementSessionIndex{};
    }

    InvalidateLoadedSession();
}

void StorageService::UpdateIndexMetadata(
    uint8_t index,
    const StoredWiFiMeasurementSession &session)
{
    if (index >= savedSessionCount &&
        !(index == 0 && savedSessionCount == 0))
    {
        return;
    }

    StoredWiFiMeasurementSessionIndex &entry =
        savedSessionIndex[index];

    entry.available = true;
    entry.sessionId = session.sessionId;
    entry.metadataLoaded = true;
    entry.capturedTimeValid =
        session.capturedTimeValid;
    entry.capturedEpoch =
        session.capturedEpoch;
    entry.networkCount =
        session.summary.networkCount;
    entry.observedNetworkCount =
        session.summary.observedNetworkCount;
    entry.bestChannel =
        session.summary.recommendation.bestChannel;
    entry.confidence =
        session.summary.recommendation.confidence;
}

void StorageService::InvalidateLoadedSession()
{
    loadedSession =
        emptyStoredSession;
    loadedSessionIndex =
        InvalidLoadedSessionIndex;
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
