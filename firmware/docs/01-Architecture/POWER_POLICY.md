# SentinelOS Power Policy

## Purpose

`PowerPolicy` defines how SentinelOS should behave under different power conditions.

It contains configuration only. It does not read hardware, control the display, or enter sleep directly.

`PowerManager` will evaluate the active policy and coordinate `PowerService`, `SleepService`, and user activity.

---

## Responsibilities

`PowerPolicy` defines:

- Whether automatic power saving is enabled
- Display-dim timeout
- Display-off timeout
- Deep-sleep timeout
- Whether automatic sleep is allowed on USB
- Whether active operations may block sleep

---

## Power Modes

### USB Mode

SentinelOS should remain fully active while connected to USB.

Default behavior:

- Automatic dimming disabled
- Automatic display-off disabled
- Automatic deep sleep disabled
- Manual deep sleep remains available

### Battery Mode

SentinelOS should conserve power during field use.

Initial default behavior:

- Dim display after 30 seconds
- Turn display off after 60 seconds
- Enter deep sleep after 120 seconds
- Reset the idle timer on user activity
- Prevent deep sleep while a wake lock is active

---

## Proposed Data Model

```cpp
struct PowerPolicy
{
    bool automaticPowerSavingEnabled = true;
    bool allowAutomaticSleepOnUsb = false;

    uint32_t dimTimeoutMs = 30000;
    uint32_t displayOffTimeoutMs = 60000;
    uint32_t deepSleepTimeoutMs = 120000;
};