#include "TimeService.h"

#include <Arduino.h>
#include <Wire.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>

namespace
{
constexpr const char *FixedTimeZone = "AST4";
constexpr const char *FixedTimeZoneLabel = "UTC-04:00";

int GetMonthFromBuildDate(const char *date)
{
    if (date == nullptr)
    {
        return 0;
    }

    static constexpr const char *Months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    for (uint8_t index = 0; index < 12; ++index)
    {
        if (std::strncmp(date, Months[index], 3) == 0)
        {
            return index + 1;
        }
    }

    return 0;
}
}

bool TimeService::rtcAvailable = false;
bool TimeService::rtcTimeValid = false;
bool TimeService::systemTimeValid = false;
bool TimeService::rtcInitializedFromBuildTime = false;

void TimeService::Begin()
{
    rtcAvailable = DetectRtc();
    rtcTimeValid = false;
    systemTimeValid = false;
    rtcInitializedFromBuildTime = false;

    setenv("TZ", FixedTimeZone, 1);
    tzset();

    LocalDateTime dateTime{};
    bool clockIntegrityValid = false;

    if (rtcAvailable)
    {
        Serial.println(
            "TimeService: RTC detected at 0x51 (PCF85063)");

        EnsureRtcRunning();

        if (ReadRtc(
                dateTime,
                clockIntegrityValid) &&
            clockIntegrityValid &&
            IsDateTimeValid(dateTime))
        {
            rtcTimeValid = true;
            Serial.println(
                "TimeService: RTC time valid");
        }
        else
        {
            if (!clockIntegrityValid)
            {
                Serial.println(
                    "TimeService: RTC clock integrity invalid");
            }
            else
            {
                Serial.println(
                    "TimeService: RTC date invalid");
            }

            LocalDateTime buildDateTime{};

            if (BuildDateTime(buildDateTime) &&
                WriteRtc(buildDateTime))
            {
                delay(10);

                bool verifiedIntegrity = false;
                LocalDateTime verifiedDateTime{};

                if (ReadRtc(
                        verifiedDateTime,
                        verifiedIntegrity) &&
                    verifiedIntegrity &&
                    IsDateTimeValid(verifiedDateTime))
                {
                    dateTime = verifiedDateTime;
                    rtcTimeValid = true;
                    rtcInitializedFromBuildTime = true;

                    Serial.println(
                        "TimeService: RTC initialized from "
                        "firmware build time");
                }
                else
                {
                    Serial.println(
                        "TimeService: RTC initialization "
                        "verification failed");
                }
            }
            else
            {
                Serial.println(
                    "TimeService: RTC could not be initialized");
            }
        }
    }
    else
    {
        Serial.println(
            "TimeService: RTC unavailable at 0x51");
    }

    if (!rtcTimeValid)
    {
        LocalDateTime buildDateTime{};

        if (BuildDateTime(buildDateTime))
        {
            dateTime = buildDateTime;
            Serial.println(
                "TimeService: Using firmware build time "
                "for this boot");
        }
    }

    if (IsDateTimeValid(dateTime))
    {
        LogDateTime(
            rtcTimeValid
                ? "TimeService: RTC local"
                : "TimeService: Build local",
            dateTime);

        systemTimeValid =
            SetSystemClock(dateTime);
    }

    if (systemTimeValid)
    {
        Serial.println(
            "TimeService: System clock synchronized");
        Serial.printf(
            "TimeService: Time zone %s\n",
            FixedTimeZoneLabel);
    }
    else
    {
        Serial.println(
            "TimeService: System clock unavailable");
    }
}

bool TimeService::IsRtcAvailable()
{
    return rtcAvailable;
}

bool TimeService::IsRtcTimeValid()
{
    return rtcTimeValid;
}

bool TimeService::IsSystemTimeValid()
{
    return systemTimeValid;
}

bool TimeService::WasRtcInitializedFromBuildTime()
{
    return rtcInitializedFromBuildTime;
}

bool TimeService::GetLocalDateTime(
    LocalDateTime &dateTime)
{
    dateTime = LocalDateTime{};

    if (!systemTimeValid)
    {
        return false;
    }

    const time_t now = time(nullptr);
    struct tm localTime{};

    if (now < 0 ||
        localtime_r(&now, &localTime) == nullptr)
    {
        return false;
    }

    dateTime.year =
        static_cast<uint16_t>(localTime.tm_year + 1900);
    dateTime.month =
        static_cast<uint8_t>(localTime.tm_mon + 1);
    dateTime.day =
        static_cast<uint8_t>(localTime.tm_mday);
    dateTime.hour =
        static_cast<uint8_t>(localTime.tm_hour);
    dateTime.minute =
        static_cast<uint8_t>(localTime.tm_min);
    dateTime.second =
        static_cast<uint8_t>(localTime.tm_sec);
    dateTime.weekday =
        static_cast<uint8_t>(localTime.tm_wday);

    return IsDateTimeValid(dateTime);
}

bool TimeService::FormatLocalDateTime(
    char *buffer,
    size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    LocalDateTime dateTime{};

    if (!GetLocalDateTime(dateTime))
    {
        buffer[0] = '\0';
        return false;
    }

    const int written = std::snprintf(
        buffer,
        bufferSize,
        "%04u-%02u-%02u %02u:%02u:%02u",
        static_cast<unsigned>(dateTime.year),
        static_cast<unsigned>(dateTime.month),
        static_cast<unsigned>(dateTime.day),
        static_cast<unsigned>(dateTime.hour),
        static_cast<unsigned>(dateTime.minute),
        static_cast<unsigned>(dateTime.second));

    return written > 0 &&
        static_cast<size_t>(written) < bufferSize;
}

bool TimeService::GetEpochTime(
    uint32_t &epochSeconds)
{
    epochSeconds = 0;

    if (!systemTimeValid)
    {
        return false;
    }

    const time_t now = time(nullptr);

    if (now <= 0)
    {
        return false;
    }

    struct tm localTime{};

    if (localtime_r(
            &now,
            &localTime) == nullptr ||
        localTime.tm_year + 1900 <
            MinimumValidYear ||
        localTime.tm_year + 1900 >
            MaximumValidYear)
    {
        return false;
    }

    epochSeconds =
        static_cast<uint32_t>(now);
    return true;
}

bool TimeService::FormatEpochIsoLocal(
    uint32_t epochSeconds,
    char *buffer,
    size_t bufferSize)
{
    if (buffer == nullptr ||
        bufferSize == 0 ||
        epochSeconds == 0)
    {
        if (buffer != nullptr &&
            bufferSize > 0)
        {
            buffer[0] = '\0';
        }

        return false;
    }

    const time_t value =
        static_cast<time_t>(epochSeconds);

    struct tm localTime{};

    if (localtime_r(
            &value,
            &localTime) == nullptr)
    {
        buffer[0] = '\0';
        return false;
    }

    const int year =
        localTime.tm_year + 1900;

    if (year < MinimumValidYear ||
        year > MaximumValidYear)
    {
        buffer[0] = '\0';
        return false;
    }

    const int written = std::snprintf(
        buffer,
        bufferSize,
        "%04d-%02d-%02dT%02d:%02d:%02d-04:00",
        year,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);

    return written > 0 &&
        static_cast<size_t>(written) < bufferSize;
}

bool TimeService::FormatEpochForHistory(
    uint32_t epochSeconds,
    char *buffer,
    size_t bufferSize)
{
    if (buffer == nullptr ||
        bufferSize == 0 ||
        epochSeconds == 0)
    {
        if (buffer != nullptr &&
            bufferSize > 0)
        {
            buffer[0] = '\0';
        }

        return false;
    }

    const time_t value =
        static_cast<time_t>(epochSeconds);

    struct tm localTime{};

    if (localtime_r(
            &value,
            &localTime) == nullptr)
    {
        buffer[0] = '\0';
        return false;
    }

    const int year =
        localTime.tm_year + 1900;

    if (year < MinimumValidYear ||
        year > MaximumValidYear)
    {
        buffer[0] = '\0';
        return false;
    }

    static constexpr const char *Months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    int displayHour =
        localTime.tm_hour % 12;

    if (displayHour == 0)
    {
        displayHour = 12;
    }

    const char *meridiem =
        localTime.tm_hour >= 12
            ? "PM"
            : "AM";

    const int written = std::snprintf(
        buffer,
        bufferSize,
        "%02d %s %04d  %d:%02d %s",
        localTime.tm_mday,
        Months[localTime.tm_mon],
        year,
        displayHour,
        localTime.tm_min,
        meridiem);

    return written > 0 &&
        static_cast<size_t>(written) < bufferSize;
}

bool TimeService::DetectRtc()
{
    Wire.beginTransmission(RtcAddress);
    return Wire.endTransmission() == 0;
}

bool TimeService::ReadRtc(
    LocalDateTime &dateTime,
    bool &clockIntegrityValid)
{
    dateTime = LocalDateTime{};
    clockIntegrityValid = false;

    uint8_t registers[7] = {};

    if (!ReadRegisters(
            SecondsRegister,
            registers,
            sizeof(registers)))
    {
        return false;
    }

    clockIntegrityValid =
        (registers[0] & OscillatorStopFlag) == 0;

    dateTime.second =
        BcdToDecimal(registers[0] & 0x7F);
    dateTime.minute =
        BcdToDecimal(registers[1] & 0x7F);
    dateTime.hour =
        BcdToDecimal(registers[2] & 0x3F);
    dateTime.day =
        BcdToDecimal(registers[3] & 0x3F);
    dateTime.weekday =
        BcdToDecimal(registers[4] & 0x07);
    dateTime.month =
        BcdToDecimal(registers[5] & 0x1F);
    dateTime.year =
        static_cast<uint16_t>(
            2000 + BcdToDecimal(registers[6]));

    return true;
}

bool TimeService::WriteRtc(
    const LocalDateTime &dateTime)
{
    if (!IsDateTimeValid(dateTime) ||
        !EnsureRtcRunning())
    {
        return false;
    }

    const uint8_t registers[7] = {
        static_cast<uint8_t>(
            DecimalToBcd(dateTime.second) & 0x7F),
        DecimalToBcd(dateTime.minute),
        DecimalToBcd(dateTime.hour),
        DecimalToBcd(dateTime.day),
        static_cast<uint8_t>(dateTime.weekday & 0x07),
        DecimalToBcd(dateTime.month),
        DecimalToBcd(
            static_cast<uint8_t>(dateTime.year % 100))
    };

    return WriteRegisters(
        SecondsRegister,
        registers,
        sizeof(registers));
}

bool TimeService::EnsureRtcRunning()
{
    uint8_t control1 = 0;

    if (!ReadRegister(
            Control1Register,
            control1))
    {
        return false;
    }

    if ((control1 & StopBit) == 0)
    {
        return true;
    }

    control1 &=
        static_cast<uint8_t>(~StopBit);

    return WriteRegister(
        Control1Register,
        control1);
}

bool TimeService::SetSystemClock(
    const LocalDateTime &dateTime)
{
    if (!IsDateTimeValid(dateTime))
    {
        return false;
    }

    struct tm localTime{};
    localTime.tm_year = dateTime.year - 1900;
    localTime.tm_mon = dateTime.month - 1;
    localTime.tm_mday = dateTime.day;
    localTime.tm_hour = dateTime.hour;
    localTime.tm_min = dateTime.minute;
    localTime.tm_sec = dateTime.second;
    localTime.tm_wday = dateTime.weekday;
    localTime.tm_isdst = 0;

    const time_t epoch = mktime(&localTime);

    if (epoch < 0)
    {
        return false;
    }

    struct timeval value{};
    value.tv_sec = epoch;
    value.tv_usec = 0;

    if (settimeofday(&value, nullptr) != 0)
    {
        return false;
    }

    const time_t verifiedEpoch = time(nullptr);
    struct tm verifiedLocalTime{};

    if (localtime_r(
            &verifiedEpoch,
            &verifiedLocalTime) == nullptr)
    {
        return false;
    }

    return verifiedLocalTime.tm_year + 1900 >=
        MinimumValidYear;
}

bool TimeService::BuildDateTime(
    LocalDateTime &dateTime)
{
    dateTime = LocalDateTime{};

    int day = 0;
    int year = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    const int month =
        GetMonthFromBuildDate(__DATE__);

    if (month == 0 ||
        std::sscanf(
            __DATE__ + 4,
            "%d %d",
            &day,
            &year) != 2 ||
        std::sscanf(
            __TIME__,
            "%d:%d:%d",
            &hour,
            &minute,
            &second) != 3)
    {
        return false;
    }

    dateTime.year =
        static_cast<uint16_t>(year);
    dateTime.month =
        static_cast<uint8_t>(month);
    dateTime.day =
        static_cast<uint8_t>(day);
    dateTime.hour =
        static_cast<uint8_t>(hour);
    dateTime.minute =
        static_cast<uint8_t>(minute);
    dateTime.second =
        static_cast<uint8_t>(second);
    dateTime.weekday =
        CalculateWeekday(
            dateTime.year,
            dateTime.month,
            dateTime.day);

    return IsDateTimeValid(dateTime);
}

bool TimeService::IsDateTimeValid(
    const LocalDateTime &dateTime)
{
    if (dateTime.year < MinimumValidYear ||
        dateTime.year > MaximumValidYear ||
        dateTime.month < 1 ||
        dateTime.month > 12 ||
        dateTime.hour > 23 ||
        dateTime.minute > 59 ||
        dateTime.second > 59 ||
        dateTime.weekday > 6)
    {
        return false;
    }

    const uint8_t daysInMonth =
        GetDaysInMonth(
            dateTime.month,
            dateTime.year);

    return dateTime.day >= 1 &&
        dateTime.day <= daysInMonth;
}

bool TimeService::IsLeapYear(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) ||
        year % 400 == 0;
}

uint8_t TimeService::GetDaysInMonth(
    uint8_t month,
    uint16_t year)
{
    static constexpr uint8_t Days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month < 1 || month > 12)
    {
        return 0;
    }

    if (month == 2 && IsLeapYear(year))
    {
        return 29;
    }

    return Days[month - 1];
}

uint8_t TimeService::CalculateWeekday(
    uint16_t year,
    uint8_t month,
    uint8_t day)
{
    uint32_t adjustedYear = year;
    uint32_t adjustedMonth = month;

    if (adjustedMonth < 3)
    {
        adjustedMonth += 12;
        --adjustedYear;
    }

    const uint32_t result = (
        day +
        (13 * (adjustedMonth + 1)) / 5 +
        adjustedYear +
        adjustedYear / 4 +
        6 * (adjustedYear / 100) +
        adjustedYear / 400) % 7;

    // Zeller result: 0 = Saturday. RTC convention: 0 = Sunday.
    return static_cast<uint8_t>((result + 6) % 7);
}

uint8_t TimeService::BcdToDecimal(uint8_t value)
{
    return static_cast<uint8_t>(
        ((value >> 4) * 10) +
        (value & 0x0F));
}

uint8_t TimeService::DecimalToBcd(uint8_t value)
{
    return static_cast<uint8_t>(
        ((value / 10) << 4) |
        (value % 10));
}

bool TimeService::ReadRegister(
    uint8_t registerAddress,
    uint8_t &value)
{
    return ReadRegisters(
        registerAddress,
        &value,
        1);
}

bool TimeService::WriteRegister(
    uint8_t registerAddress,
    uint8_t value)
{
    return WriteRegisters(
        registerAddress,
        &value,
        1);
}

bool TimeService::ReadRegisters(
    uint8_t startRegister,
    uint8_t *buffer,
    size_t length)
{
    if (buffer == nullptr ||
        length == 0 ||
        length > 255)
    {
        return false;
    }

    Wire.beginTransmission(RtcAddress);
    Wire.write(startRegister);

    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    const size_t received = Wire.requestFrom(
        static_cast<int>(RtcAddress),
        static_cast<int>(length));

    if (received != length)
    {
        while (Wire.available() > 0)
        {
            Wire.read();
        }

        return false;
    }

    for (size_t index = 0;
         index < length;
         ++index)
    {
        if (Wire.available() <= 0)
        {
            return false;
        }

        buffer[index] =
            static_cast<uint8_t>(Wire.read());
    }

    return true;
}

bool TimeService::WriteRegisters(
    uint8_t startRegister,
    const uint8_t *buffer,
    size_t length)
{
    if (buffer == nullptr ||
        length == 0 ||
        length > 255)
    {
        return false;
    }

    Wire.beginTransmission(RtcAddress);
    Wire.write(startRegister);

    for (size_t index = 0;
         index < length;
         ++index)
    {
        Wire.write(buffer[index]);
    }

    return Wire.endTransmission() == 0;
}

void TimeService::LogDateTime(
    const char *prefix,
    const LocalDateTime &dateTime)
{
    Serial.printf(
        "%s %04u-%02u-%02u %02u:%02u:%02u\n",
        prefix == nullptr ? "TimeService:" : prefix,
        static_cast<unsigned>(dateTime.year),
        static_cast<unsigned>(dateTime.month),
        static_cast<unsigned>(dateTime.day),
        static_cast<unsigned>(dateTime.hour),
        static_cast<unsigned>(dateTime.minute),
        static_cast<unsigned>(dateTime.second));
}
