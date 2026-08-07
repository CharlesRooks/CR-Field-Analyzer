#pragma once

#include <stddef.h>
#include <stdint.h>

struct LocalDateTime
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t weekday = 0;
};

class TimeService
{
public:
    // Initializes the RTC-backed local clock. The LilyGO hardware
    // library must already have initialized the shared I2C bus.
    static void Begin();

    static bool IsRtcAvailable();
    static bool IsRtcTimeValid();
    static bool IsSystemTimeValid();
    static bool WasRtcInitializedFromBuildTime();

    static bool GetLocalDateTime(
        LocalDateTime &dateTime);

    static bool FormatLocalDateTime(
        char *buffer,
        size_t bufferSize);

    static bool GetEpochTime(
        uint32_t &epochSeconds);

    static bool FormatEpochIsoLocal(
        uint32_t epochSeconds,
        char *buffer,
        size_t bufferSize);

    static bool FormatEpochForHistory(
        uint32_t epochSeconds,
        char *buffer,
        size_t bufferSize);

private:
    static constexpr uint8_t RtcAddress = 0x51;
    static constexpr uint8_t Control1Register = 0x00;
    static constexpr uint8_t SecondsRegister = 0x04;
    static constexpr uint8_t OscillatorStopFlag = 0x80;
    static constexpr uint8_t StopBit = 0x20;
    static constexpr uint16_t MinimumValidYear = 2024;
    static constexpr uint16_t MaximumValidYear = 2099;

    static bool rtcAvailable;
    static bool rtcTimeValid;
    static bool systemTimeValid;
    static bool rtcInitializedFromBuildTime;

    static bool DetectRtc();
    static bool ReadRtc(
        LocalDateTime &dateTime,
        bool &clockIntegrityValid);
    static bool WriteRtc(
        const LocalDateTime &dateTime);
    static bool EnsureRtcRunning();

    static bool SetSystemClock(
        const LocalDateTime &dateTime);
    static bool BuildDateTime(
        LocalDateTime &dateTime);

    static bool IsDateTimeValid(
        const LocalDateTime &dateTime);
    static bool IsLeapYear(uint16_t year);
    static uint8_t GetDaysInMonth(
        uint8_t month,
        uint16_t year);
    static uint8_t CalculateWeekday(
        uint16_t year,
        uint8_t month,
        uint8_t day);

    static uint8_t BcdToDecimal(uint8_t value);
    static uint8_t DecimalToBcd(uint8_t value);

    static bool ReadRegister(
        uint8_t registerAddress,
        uint8_t &value);
    static bool WriteRegister(
        uint8_t registerAddress,
        uint8_t value);
    static bool ReadRegisters(
        uint8_t startRegister,
        uint8_t *buffer,
        size_t length);
    static bool WriteRegisters(
        uint8_t startRegister,
        const uint8_t *buffer,
        size_t length);

    static void LogDateTime(
        const char *prefix,
        const LocalDateTime &dateTime);
};
