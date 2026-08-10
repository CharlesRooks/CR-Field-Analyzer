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
    static bool IsActive();

    static UsbStorageState GetState();

    // Milestone 10.19B remains intentionally read-only. SentinelOS
    // suspends its own storage writes while the host has the media.
    static bool EnterReadOnlyMode();
    static void ExitReadOnlyMode();

    static uint32_t GetReadRequestCount();
    static uint32_t GetRejectedWriteCount();

private:
    static UsbStorageState state;
    static uint32_t readRequestCount;
    static uint32_t rejectedWriteCount;
};
