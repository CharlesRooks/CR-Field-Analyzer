#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <SD.h>
#include <USB.h>
#include <USBMSC.h>

namespace
{
LilyGo_Class amoled;
USBMSC massStorage;

constexpr uint16_t ExpectedSectorSize = 512;

volatile uint32_t readRequestCount = 0;
volatile uint32_t rejectedWriteCount = 0;
volatile bool hostEjected = false;

int32_t ReadSdBlocks(
    uint32_t lba,
    uint32_t offset,
    void *buffer,
    uint32_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return 0;
    }

    const uint32_t sectorSize = SD.sectorSize();

    if (sectorSize != ExpectedSectorSize || offset >= sectorSize)
    {
        return -1;
    }

    uint8_t *destination =
        static_cast<uint8_t *>(buffer);

    uint32_t remaining = bufferSize;
    uint32_t currentLba = lba;
    uint32_t currentOffset = offset;
    uint8_t sector[ExpectedSectorSize];

    while (remaining > 0)
    {
        if (!SD.readRAW(sector, currentLba))
        {
            return -1;
        }

        const uint32_t available =
            sectorSize - currentOffset;
        const uint32_t copyLength =
            remaining < available
                ? remaining
                : available;

        memcpy(
            destination,
            sector + currentOffset,
            copyLength);

        destination += copyLength;
        remaining -= copyLength;
        ++currentLba;
        currentOffset = 0;
    }

    ++readRequestCount;
    return static_cast<int32_t>(bufferSize);
}

int32_t RejectSdWrites(
    uint32_t,
    uint32_t,
    uint8_t *,
    uint32_t)
{
    // Milestone 10.19A is deliberately read-only. Windows may issue
    // housekeeping writes while mounting a FAT volume; rejecting every
    // WRITE10 request proves transport without risking card corruption.
    ++rejectedWriteCount;
    return -1;
}

bool HandleStartStop(
    uint8_t,
    bool start,
    bool loadEject)
{
    if (loadEject && !start)
    {
        hostEjected = true;
        massStorage.mediaPresent(false);
    }

    return true;
}

void PrintCardInfo()
{
    Serial.printf(
        "USB MSC POC: SD card size %llu MB\n",
        static_cast<unsigned long long>(
            SD.cardSize() / (1024ULL * 1024ULL)));

    Serial.printf(
        "USB MSC POC: %u sectors x %u bytes\n",
        static_cast<unsigned>(SD.numSectors()),
        static_cast<unsigned>(SD.sectorSize()));
}
}

void setup()
{
    Serial.begin(115200);
    delay(2500);

    Serial.println();
    Serial.println("SentinelOS Milestone 10.19A");
    Serial.println("USB Mass Storage read-only proof of concept");

    if (!amoled.begin())
    {
        Serial.println(
            "USB MSC POC: LilyGO initialization failed");

        while (true)
        {
            delay(1000);
        }
    }

    if (SD.cardType() == CARD_NONE ||
        SD.numSectors() == 0 ||
        SD.sectorSize() == 0)
    {
        Serial.println(
            "USB MSC POC: SD card unavailable");

        while (true)
        {
            delay(1000);
        }
    }

    PrintCardInfo();

    if (SD.sectorSize() != ExpectedSectorSize)
    {
        Serial.printf(
            "USB MSC POC: Unsupported sector size %u\n",
            static_cast<unsigned>(SD.sectorSize()));

        while (true)
        {
            delay(1000);
        }
    }

    massStorage.vendorID("SENTINEL");
    massStorage.productID("SentinelOS SD");
    massStorage.productRevision("0.1");
    massStorage.onRead(ReadSdBlocks);
    massStorage.onWrite(RejectSdWrites);
    massStorage.onStartStop(HandleStartStop);

    if (!massStorage.begin(
            static_cast<uint32_t>(SD.numSectors()),
            static_cast<uint16_t>(SD.sectorSize())))
    {
        Serial.println(
            "USB MSC POC: USBMSC.begin failed");

        while (true)
        {
            delay(1000);
        }
    }

    massStorage.mediaPresent(true);

    Serial.println(
        "USB MSC POC: Media exposed to host READ-ONLY");
    Serial.println(
        "USB MSC POC: Open the SENTINEL volume in Windows");
    Serial.println(
        "USB MSC POC: Do not format the volume if Windows prompts");
}

void loop()
{
    static uint32_t lastReportMs = 0;
    static uint32_t previousReads = 0;
    static uint32_t previousRejectedWrites = 0;

    const uint32_t now = millis();

    if (now - lastReportMs >= 5000)
    {
        lastReportMs = now;

        if (readRequestCount != previousReads ||
            rejectedWriteCount != previousRejectedWrites)
        {
            previousReads = readRequestCount;
            previousRejectedWrites = rejectedWriteCount;

            Serial.printf(
                "USB MSC POC: reads=%lu, rejected writes=%lu\n",
                static_cast<unsigned long>(readRequestCount),
                static_cast<unsigned long>(rejectedWriteCount));
        }

        if (hostEjected)
        {
            Serial.println(
                "USB MSC POC: Host ejected media safely");
            hostEjected = false;
        }
    }

    delay(10);
}
