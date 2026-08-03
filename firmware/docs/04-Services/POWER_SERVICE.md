# PowerService

## Purpose

`PowerService` is the single source of truth for power telemetry within SentinelOS.

It abstracts the LilyGO/BQ25896 power-management hardware and exposes stable, cached, hardware-independent data to managers, views, and widgets.

Application pages and UI components must not access the PMU directly.

`PowerService` reports power state. It does not own display dimming, display-off, deep-sleep timing, or power-policy decisions.

---

## Responsibilities

`PowerService` is responsible for:

- Battery connection status
- Battery voltage
- Estimated battery percentage
- Charging status
- USB/VBUS connection status
- System supply voltage
- Cached power-state information
- Power-status formatting
- Hardware-safe PMU access

---

## Power Architecture

```text
LilyGO / BQ25896 Hardware
            │
            ▼
       PowerService
    Cached PowerInfo
       │         │
       │         └────────► HeaderBar / SystemDashboardView
       │
       ▼
    PowerManager
       │
       ├────────► DisplayService
       └────────► SleepService
```

Ownership:

- `PowerService` owns PMU capability and telemetry.
- `PowerManager` owns policy and power-state coordination.
- `DisplayService` owns brightness and display on/off capability.
- `SleepService` owns deep sleep and wake-source configuration.
- `PowerPolicy` contains configuration only.

---

## Update Model

Power information is not read from the PMU every time a consumer requests a value.

`PowerService` follows a cached-state model:

```text
PowerService::Update()
        │
        ▼
Read PMU values once
        │
        ▼
Update cached PowerInfo
        │
        ▼
Managers and UI read cached values
```

The current refresh interval is one second.

---

## PowerInfo Data Model

```cpp
struct PowerInfo
{
    bool batteryConnected;
    bool charging;
    bool usbConnected;

    uint16_t batteryVoltageMv;
    uint16_t usbVoltageMv;
    uint16_t systemVoltageMv;

    uint8_t batteryPercent;
};
```

### Field Definitions

| Field | Description |
|---|---|
| `batteryConnected` | Indicates whether a battery is detected |
| `charging` | Indicates whether the PMU reports a validated charging state |
| `usbConnected` | Indicates whether external USB/VBUS power is present |
| `batteryVoltageMv` | Battery voltage in millivolts |
| `usbVoltageMv` | USB/VBUS voltage in millivolts |
| `systemVoltageMv` | System supply voltage in millivolts |
| `batteryPercent` | Estimated battery charge from 0–100% |

---

## Public API

```cpp
class PowerService
{
public:
    static void Begin(LilyGo_AMOLED *device);
    static void Update();

    static const PowerInfo &GetInfo();

    static bool IsBatteryConnected();
    static bool IsCharging();
    static bool IsUSBConnected();

    static uint16_t GetBatteryVoltageMv();
    static uint16_t GetUSBVoltageMv();
    static uint16_t GetSystemVoltageMv();

    static uint8_t GetBatteryPercent();

    static void FormatStatus(
        char *buffer,
        size_t bufferSize);
};
```

The exact public header remains the authoritative API contract.

---

## Battery Percentage Estimation

Battery percentage is estimated from measured single-cell LiPo voltage.

The current implementation uses a calibrated nonlinear, piecewise voltage-to-percentage curve rather than a linear mapping.

The implementation:

1. Reads battery voltage from the BQ25896.
2. Validates the voltage as a plausible single-cell LiPo reading.
3. Applies charging-state compensation.
4. Preserves the last valid discharge estimate while charging where appropriate.
5. Smooths voltage-settling changes to reduce percentage oscillation.
6. Locates the voltage between calibrated curve points.
7. Interpolates the percentage between those points.
8. Clamps the final estimate to 0–100%.

This provides a practical field indicator but is not equivalent to a dedicated coulomb-counting fuel gauge.

Future accuracy improvements may include:

- Battery characterization under known loads
- Temperature compensation
- Runtime modelling
- Battery-health estimation
- A dedicated fuel-gauge IC

---

## Charging-State Handling

Charging state is based on validated BQ25896 charge states.

Charger enablement is conditional on the presence of both:

- USB/VBUS power
- A connected battery

The battery percentage logic compensates for elevated charging voltage and avoids treating charging voltage as a direct discharge-state measurement.

---

## Refresh Interval

The default refresh interval is:

```text
1000 ms
```

This provides responsive UI updates without excessive PMU polling.

---

## Error Handling

If no battery is connected:

- `batteryConnected` is `false`
- `batteryPercent` is `0`
- Battery voltage may be `0`
- The UI displays external-power status where applicable

If the PMU device is unavailable, the service returns safe default values and avoids blocking SentinelOS.

---

## UI Integration

### HeaderBar

The HeaderBar consumes formatted, cached PowerService status.

Example:

```text
SentinelOS                     87%
Dashboard
```

The status may reflect battery, charging, or USB operation.

### SystemDashboardView

The dashboard power tile may display:

- Battery percentage
- Battery voltage
- Charging state
- USB power state
- Current power source

### Future Power Management View

A dedicated view may display:

- Battery percentage
- Battery voltage
- USB voltage
- System voltage
- Charging state
- Power source
- Estimated runtime
- Current policy
- Brightness controls
- Sleep controls

---

## PowerManager Integration

`PowerManager` reads `PowerService::IsUSBConnected()` to select the active policy.

Current behaviour:

### USB

- Automatic power saving disabled
- Display restored to normal active brightness on transition
- Idle timer reset on transition

### Battery

- Display dims after 30 seconds
- Display turns off after 60 seconds
- Deep sleep begins after 120 seconds
- Idle timer starts fresh when USB is disconnected

Power-management actions are delegated to `DisplayService` and `SleepService`; they are not performed by `PowerService`.

---

## Future Expansion

Future PowerService capabilities may include:

- Low-battery warning thresholds
- Critical-battery state reporting
- Battery runtime estimation
- Charge-current information
- Battery-health estimation
- Battery-cycle tracking
- Power statistics

Display control, sleep control, and policy remain outside PowerService.

---

## Design Rules

1. UI components must not access the PMU directly.
2. PMU values are read during `PowerService::Update()`.
3. Public getters return cached values.
4. Hardware-specific PMU logic remains inside PowerService.
5. Missing hardware must not stop SentinelOS from running.
6. Battery percentage is identified as an estimate unless a fuel gauge is available.
7. PowerService reports state; PowerManager owns policy.
8. DisplayService owns display capability.
9. SleepService owns deep-sleep capability.

---

## Implementation Status

### Completed

- PowerService architecture and public API
- PowerInfo cached data model
- BQ25896 measurement initialization
- Battery voltage monitoring
- USB/VBUS voltage monitoring
- System voltage monitoring
- Battery connection inference
- USB and battery power-source identification
- One-second cached PMU polling
- Centralized power-status formatting
- System Dashboard power tile
- Persistent HeaderBar power indicator
- Live switching between USB and battery operation
- BQ25896 charging-state validation
- Conditional charger enablement
- Charging-state reporting
- Charging-voltage percentage compensation
- Battery percentage smoothing
- Piecewise LiPo voltage-to-percentage curve
- Interpolated battery percentage estimation
- Calibrated nonlinear battery discharge estimation
- Integration with USB and battery power-policy selection
- Automatic display dimming and display-off through PowerManager
- Automatic battery deep sleep through PowerManager and SleepService
- Power-source transition handling
- Normal-brightness restoration across dim and display-off states

### Remaining

- Low-battery warnings
- Critical-battery handling
- Power statistics
- Dedicated Power Management view
- Battery-health and runtime estimation
