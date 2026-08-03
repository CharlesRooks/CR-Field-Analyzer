# SentinelOS Power Policy

## Purpose

`PowerPolicy` defines how SentinelOS behaves under different power conditions.

It contains configuration only. It does not:

- Read PMU hardware
- Detect the active power source
- Control display brightness
- Turn the display on or off
- Enter deep sleep

`PowerManager` selects the active policy and coordinates `PowerService`, `DisplayService`, `SleepService`, and user-activity events.

---

## Responsibilities

`PowerPolicy` defines:

- Whether automatic power saving is enabled
- Whether automatic sleep is allowed while USB power is connected
- Whether display dimming is allowed
- Whether display-off is allowed
- Whether deep sleep is allowed
- Display-dim timeout
- Display-off timeout
- Deep-sleep timeout

---

## Implemented Data Model

```cpp
struct PowerPolicy
{
    bool automaticPowerSavingEnabled = true;
    bool allowAutomaticSleepOnUsb = false;
    bool allowDisplayDimming = true;
    bool allowDisplaySleep = true;
    bool allowDeepSleep = true;

    uint32_t dimTimeoutMs = 30000;
    uint32_t displayOffTimeoutMs = 60000;
    uint32_t deepSleepTimeoutMs = 120000;
};
```

---

## Active Policies

`PowerManager` maintains two policy instances:

- USB policy
- Battery policy

The active policy is selected from the cached USB/VBUS state reported by `PowerService`.

---

## USB Policy

SentinelOS remains fully active while connected to USB.

Current defaults:

```text
automaticPowerSavingEnabled = false
allowAutomaticSleepOnUsb    = false
allowDisplayDimming          = false
allowDisplaySleep            = false
allowDeepSleep               = false
dimTimeoutMs                 = 0
displayOffTimeoutMs          = 0
deepSleepTimeoutMs           = 0
```

Behaviour:

- Automatic dimming is disabled
- Automatic display-off is disabled
- Automatic deep sleep is disabled
- Manual BOOT-button deep sleep remains available
- Connecting USB restores the display to its normal active brightness
- Connecting USB resets the idle timer

---

## Battery Policy

SentinelOS conserves power during field operation.

Current defaults:

```text
automaticPowerSavingEnabled = true
allowAutomaticSleepOnUsb    = false
allowDisplayDimming          = true
allowDisplaySleep            = true
allowDeepSleep               = true
dimTimeoutMs                 = 30000
displayOffTimeoutMs          = 60000
deepSleepTimeoutMs           = 120000
```

Behaviour:

```text
30 seconds idle  → Display dims
60 seconds idle  → Display turns off
120 seconds idle → Device enters deep sleep
```

User activity:

- Resets the idle timer
- Restores normal brightness from the dimmed state
- Turns the display on at normal brightness from the off state

Disconnecting USB:

- Activates the battery policy
- Resets the idle timer
- Keeps the display active
- Starts the battery-policy timeout sequence from the transition

---

## Policy Evaluation

`PowerManager::Update()`:

1. Selects the active policy from the current USB state.
2. Detects power-source transitions.
3. Resets idle timing when a transition occurs.
4. Restores the display to `Active` when a transition occurs.
5. Applies the active policy to the current idle time and display state.

If `automaticPowerSavingEnabled` is `false`, automatic timeout actions are skipped.

---

## Policy Transition Rules

### Battery to USB

- Select USB policy
- Reset idle time
- Restore normal brightness or turn the display on
- Set display state to `Active`

### USB to Battery

- Select battery policy
- Reset idle time
- Keep the display at normal active brightness
- Begin battery timeout counting from the transition

---

## Wake Locks

Wake locks are not currently implemented.

Future long-running operations such as Wi-Fi scans, exports, or firmware updates may require a wake-lock mechanism to temporarily prevent display-off or deep sleep.

---

## Design Rules

1. Policy fields configure behaviour; they do not perform actions.
2. Managers own policy selection and enforcement.
3. Services own hardware capability.
4. User activity resets policy timing through `MessageBus`.
5. Power-source transitions must not inherit stale idle time.
6. Display dimming must not overwrite the saved normal brightness.
7. Automatic deep sleep is permitted only when the active policy allows it.
