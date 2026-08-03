# SentinelOS Power Architecture

## Purpose

This document defines the implemented power-management architecture for SentinelOS.

The design separates:

- Power telemetry and hardware state
- Power-saving policy
- Display power capability
- Deep-sleep capability
- Power-policy coordination
- User-activity events

No single component owns every aspect of power management.

---

## Architecture Overview

```text
LilyGO AMOLED / BQ25896 Hardware
             │
             ▼
        PowerService
   Cached power telemetry
             │
             ▼
        PowerManager
  Selects and enforces policy
       │             │
       ▼             ▼
DisplayService   SleepService
Brightness and   Deep sleep and
display state    wake-source setup
       ▲
       │
 UserActivity
       │
   MessageBus
       ▲
       │
InputManager and SentinelOS BOOT-button handling
```

---

## Component Responsibilities

### PowerService

`PowerService` owns power telemetry and PMU access.

It provides cached information such as:

- Battery connection state
- Battery voltage
- Estimated battery percentage
- Charging state
- USB/VBUS connection state
- System supply voltage

`PowerService` does not own display dimming, display-off behaviour, deep-sleep timing, or power-policy decisions.

### PowerPolicy

`PowerPolicy` is configuration only.

It defines:

- Whether automatic power saving is enabled
- Whether display dimming is allowed
- Whether display-off is allowed
- Whether deep sleep is allowed
- Whether automatic sleep is allowed while USB power is connected
- Dim timeout
- Display-off timeout
- Deep-sleep timeout

It does not read hardware or perform power actions.

### PowerManager

`PowerManager` owns power-saving policy and the display power-state machine.

It:

- Selects the USB or battery policy
- Tracks the last user-activity time
- Maintains `DisplayPowerState`
- Requests display dimming, restoration, turn-off, and turn-on
- Requests automatic deep sleep
- Resets policy timing when the power source changes
- Restores the display to full active brightness when the power source changes

### DisplayService

`DisplayService` owns display power capability.

It:

- Sets and reports brightness
- Dims the display
- Restores the saved normal brightness
- Turns the display off
- Turns the display on
- Tracks whether the display is on
- Tracks whether the current brightness is the temporary dimmed level

The service preserves the normal brightness separately from the dimmed brightness. Turning the display off while dimmed must not overwrite the saved normal brightness.

### SleepService

`SleepService` owns deep-sleep capability.

It:

- Detects the wake reason
- Configures GPIO0 as the BOOT-button wake source
- Prevents immediate wake by waiting for an already-held BOOT button to be released
- Enters ESP32-S3 deep sleep

### MessageBus and User Activity

`InputManager` publishes `UserActivity` for completed touch gestures.

`SentinelOS` publishes `UserActivity` when the physical BOOT button is pressed.

`PowerManager` subscribes to `UserActivity` and:

- Resets the idle timer
- Restores full brightness from the dimmed state
- Turns the display on at the saved normal brightness from the off state

---

## Power Policies

### USB Policy

While USB power is connected:

- Automatic power saving is disabled
- Automatic display dimming is disabled
- Automatic display-off is disabled
- Automatic deep sleep is disabled
- Manual deep sleep through the BOOT-button long press remains available

### Battery Policy

While operating on battery:

```text
30 seconds idle  → Display dims
60 seconds idle  → Display turns off
120 seconds idle → Device enters deep sleep
```

User activity resets the sequence.

---

## Display Power-State Machine

```text
                 UserActivity
             ┌──────────────────┐
             │                  ▼
Active ──30 s idle──► Dimmed ──60 s total idle──► Off
  ▲                    │                           │
  │                    │ UserActivity              │ UserActivity
  │                    └───────────────────────────┤
  │                                                ▼
  └──────── Restore normal brightness / Turn on ───┘

Off ──120 s total idle──► Deep Sleep
```

`DisplayPowerState` values:

- `Active`
- `Dimmed`
- `Off`

Deep sleep is not represented as another display state because entering deep sleep stops normal firmware execution.

---

## Power-Source Transitions

Power-source changes are treated as activity boundaries.

### Battery to USB

When USB is connected:

- The USB policy becomes active
- The idle timer is reset
- A dimmed display returns to normal brightness
- An off display turns on at normal brightness
- The display state becomes `Active`
- Automatic power saving remains disabled while USB stays connected

### USB to Battery

When USB is disconnected:

- The battery policy becomes active
- The idle timer is reset
- The display remains active at normal brightness
- Battery-policy timeouts begin from the transition

This prevents an old USB-session idle time from causing immediate dimming, display-off, or deep sleep after disconnection.

---

## Manual Deep Sleep

Holding the BOOT button for approximately two seconds invokes `SleepService::EnterDeepSleep()` directly.

The BOOT button is active-low. `SleepService` waits until the button is released before starting deep sleep so the configured wake condition is not already active.

---

## Wake Behaviour

The implemented wake source is GPIO0 through the BOOT button.

After waking, the ESP32-S3 restarts SentinelOS and `SleepService` reports the wake reason.

The current implementation does not provide touch wake from deep sleep.

---

## Serial Diagnostics

Power-policy transitions and automatic deep-sleep entry may be logged through `Serial`.

Because the device uses the same USB connection for power and serial communication, USB-to-battery transition messages cannot normally be observed after the cable is disconnected. Functional device behaviour is therefore the authoritative validation for that transition.

---

## Message Types

The current power implementation uses `UserActivity` as an active MessageBus flow.

Power, display, and sleep message types remain reserved for future event flows. The present implementation coordinates display and sleep capabilities directly through `PowerManager`.

---

## Design Rules

1. `PowerService` owns PMU access and cached power telemetry.
2. `PowerPolicy` contains configuration only.
3. `PowerManager` owns power-saving decisions and policy transitions.
4. `DisplayService` owns brightness and display on/off capability.
5. `SleepService` owns deep sleep and wake-source configuration.
6. UI components must not access PMU or display hardware directly.
7. User activity must reach `PowerManager` through `MessageBus`.
8. The normal brightness level must not be overwritten by the temporary dimmed level.
9. Power-source transitions reset idle timing and normalize the display to `Active`.
10. Automatic deep sleep is enabled on battery and disabled on USB under the current default policies.

---

## Implementation Status

Completed:

- Cached PowerService telemetry
- USB and battery policy selection
- Display dimming
- Display-off
- Brightness restoration
- Automatic battery deep sleep
- Manual BOOT-button deep sleep
- BOOT-button wake
- User-activity reset through MessageBus
- Battery-to-USB transition handling
- USB-to-battery transition handling
- Normal-brightness preservation across dim and display-off states

Future work:

- Low-battery warnings
- Critical-battery handling
- Wake locks for long-running operations
- Runtime estimation
- Battery-health statistics
- Additional wake sources
- Dedicated power-management UI
