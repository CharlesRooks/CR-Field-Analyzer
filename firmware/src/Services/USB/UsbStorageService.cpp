#include "UsbStorageService.h"

#include "../Storage/StorageService.h"

#include <Arduino.h>

#ifndef SENTINELOS_USB_STORAGE_FEATURE
#define SENTINELOS_USB_STORAGE_FEATURE 0
#endif

UsbStorageState UsbStorageService::state =
    UsbStorageState::Unavailable;

uint32_t UsbStorageService::readRequestCount = 0;
uint32_t UsbStorageService::writeRequestCount = 0;
uint32_t UsbStorageService::failedWriteRequestCount = 0;

#if SENTINELOS_USB_STORAGE_FEATURE

#include <SD.h>
#include <USBMSC.h>
#include <cstring>

namespace
{
USBMSC massStorage;

constexpr uint16_t ExpectedSectorSize = 512;

volatile bool hostEjectPending = false;
volatile uint32_t pendingReadRequestCount = 0;
volatile uint32_t pendingWriteRequestCount = 0;
volatile uint32_t pendingFailedWriteRequestCount = 0;

bool TransferRangeIsValid(
    uint32_t lba,
    uint32_t offset,
    uint32_t bufferSize)
{
    const uint32_t sectorSize = SD.sectorSize();
    const uint32_t sectorCount = SD.numSectors();

    if (sectorSize != ExpectedSectorSize ||
        sectorCount == 0 ||
        offset >= sectorSize)
    {
        return false;
    }

    const uint64_t startByte =
        static_cast<uint64_t>(lba) * sectorSize +
        offset;

    const uint64_t mediaBytes =
        static_cast<uint64_t>(sectorCount) * sectorSize;

    return startByte <= mediaBytes &&
        static_cast<uint64_t>(bufferSize) <=
            mediaBytes - startByte;
}

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

    if (!TransferRangeIsValid(
            lba,
            offset,
            bufferSize))
    {
        return -1;
    }

    const uint32_t sectorSize = SD.sectorSize();

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

        std::memcpy(
            destination,
            sector + currentOffset,
            copyLength);

        destination += copyLength;
        remaining -= copyLength;
        ++currentLba;
        currentOffset = 0;
    }

    ++pendingReadRequestCount;
    return static_cast<int32_t>(bufferSize);
}

int32_t WriteSdBlocks(
    uint32_t lba,
    uint32_t offset,
    uint8_t *buffer,
    uint32_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return 0;
    }

    if (!TransferRangeIsValid(
            lba,
            offset,
            bufferSize))
    {
        ++pendingFailedWriteRequestCount;
        return -1;
    }

    const uint32_t sectorSize = SD.sectorSize();

    const uint8_t *source = buffer;
    uint32_t remaining = bufferSize;
    uint32_t currentLba = lba;
    uint32_t currentOffset = offset;
    uint8_t sector[ExpectedSectorSize];

    while (remaining > 0)
    {
        const uint32_t available =
            sectorSize - currentOffset;

        const uint32_t copyLength =
            remaining < available
                ? remaining
                : available;

        const bool fullSectorWrite =
            currentOffset == 0 &&
            copyLength == sectorSize;

        if (!fullSectorWrite)
        {
            // Preserve bytes outside the host's requested range when
            // TinyUSB supplies a partial-sector write.
            if (!SD.readRAW(sector, currentLba))
            {
                ++pendingFailedWriteRequestCount;
                return -1;
            }
        }

        std::memcpy(
            sector + currentOffset,
            source,
            copyLength);

        if (!SD.writeRAW(sector, currentLba))
        {
            ++pendingFailedWriteRequestCount;
            return -1;
        }

        source += copyLength;
        remaining -= copyLength;
        ++currentLba;
        currentOffset = 0;
    }

    ++pendingWriteRequestCount;
    return static_cast<int32_t>(bufferSize);
}

bool HandleStartStop(
    uint8_t,
    bool start,
    bool loadEject)
{
    if (loadEject && !start)
    {
        hostEjectPending = true;
        massStorage.mediaPresent(false);
    }

    return true;
}
}

#endif

void UsbStorageService::Begin()
{
    state = UsbStorageState::Unavailable;
    readRequestCount = 0;
    writeRequestCount = 0;
    failedWriteRequestCount = 0;

#if SENTINELOS_USB_STORAGE_FEATURE
    hostEjectPending = false;
    pendingReadRequestCount = 0;
    pendingWriteRequestCount = 0;
    pendingFailedWriteRequestCount = 0;

    if (!StorageService::IsAvailable())
    {
        Serial.println(
            "UsbStorageService: SD card unavailable");
        return;
    }

    if (SD.numSectors() == 0 ||
        SD.sectorSize() != ExpectedSectorSize)
    {
        Serial.printf(
            "UsbStorageService: Unsupported SD geometry "
            "(%u sectors x %u bytes)\n",
            static_cast<unsigned>(SD.numSectors()),
            static_cast<unsigned>(SD.sectorSize()));
        return;
    }

    massStorage.vendorID("SENTINEL");
    massStorage.productID("SentinelOS SD");
    massStorage.productRevision("0.4");
    massStorage.onRead(ReadSdBlocks);
    massStorage.onWrite(WriteSdBlocks);
    massStorage.onStartStop(HandleStartStop);

    if (!massStorage.begin(
            static_cast<uint32_t>(SD.numSectors()),
            static_cast<uint16_t>(SD.sectorSize())))
    {
        Serial.println(
            "UsbStorageService: USBMSC.begin failed");
        return;
    }

    // Keep the medium absent until the user explicitly enters USB
    // Transfer Mode. This prevents Windows from taking ownership of
    // the filesystem during normal SentinelOS operation.
    massStorage.mediaPresent(false);

    state = UsbStorageState::Ready;

    Serial.printf(
        "UsbStorageService: Ready, read/write transfer media "
        "%u x %u bytes\n",
        static_cast<unsigned>(SD.numSectors()),
        static_cast<unsigned>(SD.sectorSize()));
#else
    Serial.println(
        "UsbStorageService: Feature not built in this environment");
#endif
}

void UsbStorageService::Update()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    readRequestCount = pendingReadRequestCount;
    writeRequestCount = pendingWriteRequestCount;
    failedWriteRequestCount =
        pendingFailedWriteRequestCount;

    if (hostEjectPending)
    {
        hostEjectPending = false;

        if (state == UsbStorageState::Active)
        {
            // Do NOT return the mounted FAT filesystem to SentinelOS
            // after host writes. Its cached filesystem state may now be
            // stale. Keep exclusive ownership locked until a clean reboot.
            state = UsbStorageState::HostEjected;

            Serial.println(
                "UsbStorageService: Host safely ejected writable media; "
                "restart required before SentinelOS SD access resumes");
        }
    }
#endif
}

bool UsbStorageService::IsFeatureBuilt()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    return true;
#else
    return false;
#endif
}

bool UsbStorageService::IsReady()
{
    return state != UsbStorageState::Unavailable;
}

bool UsbStorageService::IsActive()
{
    // HostEjected intentionally remains active from SentinelOS' point
    // of view. Navigation and storage access stay locked until reboot.
    return state == UsbStorageState::Active ||
        state == UsbStorageState::HostEjected;
}

UsbStorageState UsbStorageService::GetState()
{
    return state;
}

bool UsbStorageService::EnterReadWriteMode()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    if (state != UsbStorageState::Ready)
    {
        return false;
    }

    if (!StorageService::IsAvailable())
    {
        state = UsbStorageState::Unavailable;
        return false;
    }

    // This existing StorageService interlock blocks all SentinelOS
    // storage mutations and Wi-Fi measurement activity. The Tools page
    // and NavigationManager additionally keep the user out of SD-backed
    // workflows while Windows owns the volume.
    StorageService::SetExternalReadOnlyAccessActive(true);

    hostEjectPending = false;
    pendingReadRequestCount = 0;
    pendingWriteRequestCount = 0;
    pendingFailedWriteRequestCount = 0;
    readRequestCount = 0;
    writeRequestCount = 0;
    failedWriteRequestCount = 0;

    state = UsbStorageState::Active;
    massStorage.mediaPresent(true);

    Serial.println(
        "UsbStorageService: USB Transfer Mode active (READ/WRITE)");

    return true;
#else
    return false;
#endif
}

void UsbStorageService::RestartAfterTransfer()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    if (state != UsbStorageState::HostEjected)
    {
        Serial.println(
            "UsbStorageService: Restart blocked - "
            "eject the drive in Windows first");
        return;
    }

    massStorage.mediaPresent(false);

    Serial.println(
        "UsbStorageService: Restarting to remount and reindex SD card");

    Serial.flush();
    delay(150);
    ESP.restart();
#endif
}

uint32_t UsbStorageService::GetReadRequestCount()
{
    return readRequestCount;
}

uint32_t UsbStorageService::GetWriteRequestCount()
{
    return writeRequestCount;
}

uint32_t UsbStorageService::GetFailedWriteRequestCount()
{
    return failedWriteRequestCount;
}
