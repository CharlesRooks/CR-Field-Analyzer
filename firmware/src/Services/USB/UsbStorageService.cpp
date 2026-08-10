#include "UsbStorageService.h"

#include "../Storage/StorageService.h"

#include <Arduino.h>

#ifndef SENTINELOS_USB_STORAGE_FEATURE
#define SENTINELOS_USB_STORAGE_FEATURE 0
#endif

UsbStorageState UsbStorageService::state =
    UsbStorageState::Unavailable;

uint32_t UsbStorageService::readRequestCount = 0;
uint32_t UsbStorageService::rejectedWriteCount = 0;

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
volatile uint32_t pendingRejectedWriteCount = 0;

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

    if (sectorSize != ExpectedSectorSize ||
        offset >= sectorSize)
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

int32_t RejectSdWrites(
    uint32_t,
    uint32_t,
    uint8_t *,
    uint32_t)
{
    ++pendingRejectedWriteCount;
    return -1;
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
    rejectedWriteCount = 0;

#if SENTINELOS_USB_STORAGE_FEATURE
    hostEjectPending = false;
    pendingReadRequestCount = 0;
    pendingRejectedWriteCount = 0;

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
    massStorage.productRevision("0.2");
    massStorage.onRead(ReadSdBlocks);
    massStorage.onWrite(RejectSdWrites);
    massStorage.onStartStop(HandleStartStop);

    if (!massStorage.begin(
            static_cast<uint32_t>(SD.numSectors()),
            static_cast<uint16_t>(SD.sectorSize())))
    {
        Serial.println(
            "UsbStorageService: USBMSC.begin failed");
        return;
    }

    // The MSC interface is part of the TinyUSB descriptor from boot,
    // but the medium remains absent until the user explicitly enters
    // USB Storage Mode from Tools.
    massStorage.mediaPresent(false);

    state = UsbStorageState::Ready;

    Serial.printf(
        "UsbStorageService: Ready, read-only media %u x %u bytes\n",
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
    rejectedWriteCount = pendingRejectedWriteCount;

    if (hostEjectPending)
    {
        hostEjectPending = false;

        if (state == UsbStorageState::Active)
        {
            state = UsbStorageState::HostEjected;
            StorageService::SetExternalReadOnlyAccessActive(false);

            Serial.println(
                "UsbStorageService: Host ejected media safely");
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
    return state == UsbStorageState::Active;
}

UsbStorageState UsbStorageService::GetState()
{
    return state;
}

bool UsbStorageService::EnterReadOnlyMode()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    if (state != UsbStorageState::Ready &&
        state != UsbStorageState::HostEjected)
    {
        return false;
    }

    if (!StorageService::IsAvailable())
    {
        state = UsbStorageState::Unavailable;
        return false;
    }

    StorageService::SetExternalReadOnlyAccessActive(true);

    hostEjectPending = false;
    state = UsbStorageState::Active;
    massStorage.mediaPresent(true);

    Serial.println(
        "UsbStorageService: USB Storage Mode active (READ-ONLY)");

    return true;
#else
    return false;
#endif
}

void UsbStorageService::ExitReadOnlyMode()
{
#if SENTINELOS_USB_STORAGE_FEATURE
    if (state == UsbStorageState::Unavailable)
    {
        return;
    }

    massStorage.mediaPresent(false);
    hostEjectPending = false;
    StorageService::SetExternalReadOnlyAccessActive(false);
    state = UsbStorageState::Ready;

    Serial.println(
        "UsbStorageService: USB Storage Mode stopped");
#endif
}

uint32_t UsbStorageService::GetReadRequestCount()
{
    return readRequestCount;
}

uint32_t UsbStorageService::GetRejectedWriteCount()
{
    return rejectedWriteCount;
}
