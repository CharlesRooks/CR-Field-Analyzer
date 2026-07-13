# PowerService

## Purpose

`PowerService` is the single source of truth for power-related information and control within SentinelOS.

It abstracts the LilyGO power-management hardware from the rest of the application and exposes stable, hardware-independent data to views, widgets, and other services.

Application pages and UI components must not access the PMU directly.

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
- Future low-power and sleep operations

---

## Architecture

```text
LilyGO Power Hardware
        |
        v
PowerService
        |
        +-- HeaderBar
        +-- SystemDashboardView
        +-- Power Management View
        +-- Low Battery Alerts
```

The UI consumes cached values from `PowerService` and never communicates directly with the power-management hardware.

---

## Update Model

Power information should not be read from the PMU every time a widget requests a value.

`PowerService` follows a cached-state model:

```text
PowerService::Update()
        |
        v
Read PMU values once
        |
        v
Update cached PowerInfo
        |
        v
UI reads cached values
```

The initial refresh interval is one second.

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
| `charging` | Indicates whether the battery is charging |
| `usbConnected` | Indicates whether external USB/VBUS power is present |
| `batteryVoltageMv` | Battery voltage in millivolts |
| `usbVoltageMv` | USB/VBUS voltage in millivolts |
| `systemVoltageMv` | System supply voltage in millivolts |
| `batteryPercent` | Estimated battery charge from 0–100% |

---

## Initial Public API

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
};
```

---

## Battery Percentage Estimation

Battery percentage is currently estimated from the measured battery voltage.

The implementation:

1. Reads the battery voltage from the BQ25896.
2. Validates that the voltage is within a plausible single-cell battery range.
3. Clamps the voltage between the configured empty and full thresholds.
4. Converts the voltage into a percentage from 0–100%.

The current implementation uses a linear voltage-based estimate.

This provides a practical battery indicator but should not be treated as laboratory-grade battery capacity measurement.

Future versions may use:

- A nonlinear lithium-battery discharge curve
- Load compensation
- Charging-state compensation
- Battery characterization data
- Battery-health estimation
- A dedicated fuel-gauge IC

---

## Initial Voltage Range

The initial implementation will use configurable values similar to:

```cpp
BATTERY_EMPTY_MV = 3300
BATTERY_FULL_MV  = 4200
```

These values may be adjusted after hardware testing with the actual battery.

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
- The UI should display external power status where applicable

If the PMU device is unavailable, the service should return safe default values and avoid blocking the application.

---

## UI Integration

### HeaderBar

The HeaderBar may display:

```text
SentinelOS                     87%
Dashboard
```

Future versions may add charging or USB indicators.

### SystemDashboardView

The system dashboard may include PowerService values such as:

- Battery percentage
- Battery voltage
- Charging state
- USB power state

### Power Management View

A future dedicated view may display:

- Battery percentage
- Battery voltage
- USB voltage
- System voltage
- Charging state
- Power source
- Estimated runtime
- Brightness controls
- Sleep controls

---

## Future Expansion

Future PowerService capabilities may include:

- Display brightness control
- Automatic brightness
- Low-battery warning thresholds
- Critical-battery shutdown
- Light sleep
- Deep sleep
- Wake-source configuration
- Battery runtime estimation
- Power profiles
- Charge-current information
- Battery-health estimation
- Battery-cycle tracking

---

## Design Rules

1. UI components must not access the PMU directly.
2. PMU values are read during `PowerService::Update()`.
3. Public getters return cached values.
4. Hardware-specific logic remains inside the service.
5. Missing hardware must not stop SentinelOS from running.
6. Battery percentage is identified as an estimate unless a fuel gauge is available.

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
- Battery percentage estimation
- One-second cached PMU polling
- Centralized power-status formatting
- System Dashboard power tile
- Persistent HeaderBar power indicator
- Live switching between USB and battery operation
- BQ25896 charging-state validation
- Conditional charger enablement when USB and battery are present
- Charging-state reporting
- Charging-voltage percentage compensation
- Battery percentage smoothing during voltage settling
- Piecewise LiPo voltage-to-percentage curve
- Interpolated battery percentage estimation
- Calibrated nonlinear battery discharge estimation

### Remaining

- Low-battery warnings
- Power statistics
- Sleep-mode preparation
- Dedicated Power Management view
- Battery-health and runtime estimation