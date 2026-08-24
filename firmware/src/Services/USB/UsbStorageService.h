#pragma once

#include <stdint.h>

enum class UsbStorageState : uint8_t
{
    Unavailable = 0,
    Ready,
    Active,
    HostEjected
};

class UsbStorageService
{
public:
    static void Begin();
    static void Update();

    static bool IsFeatureBuilt();
    static bool IsReady();

    // Returns true for the entire exclusive host-ownership window,
    // including the post-eject state before SentinelOS restarts.
    static bool IsActive();

    static UsbStorageState GetState();

    // USB Transfer Mode gives the host read/write access to the SD card.
    // SentinelOS must not use the filesystem until the host safely ejects
    // the volume and the device restarts.
    static bool EnterReadWriteMode();
    static void RestartAfterTransfer();

    static uint32_t GetReadRequestCount();
    static uint32_t GetWriteRequestCount();
    static uint32_t GetFailedWriteRequestCount();

private:
    static UsbStorageState state;
    static uint32_t readRequestCount;
    static uint32_t writeRequestCount;
    static uint32_t failedWriteRequestCount;
};
