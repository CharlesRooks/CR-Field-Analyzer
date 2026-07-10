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

The initial battery percentage will be estimated from voltage.

This is an approximation and should not be treated as laboratory-grade battery capacity measurement.

The first implementation will:

1. Read battery voltage.
2. Clamp the value between configured minimum and maximum voltages.
3. Convert the voltage into a percentage from 0–100%.

Future versions may use:

- A nonlinear lithium-battery discharge curve
- Load compensation
- Charging-state compensation
- Battery characterization data
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

## Initial Milestones

### Milestone 7.1 — PowerService Design

- Define responsibilities
- Define `PowerInfo`
- Define public API
- Document update model

### Milestone 7.2 — PowerService Framework

- Create `PowerInfo.h`
- Implement service initialization
- Implement cached state
- Add safe default values

### Milestone 7.3 — PMU Integration

- Read battery voltage
- Detect battery connection
- Detect charging
- Detect USB/VBUS power
- Read system voltage

### Milestone 7.4 — Battery Percentage

- Add voltage-based percentage estimation
- Validate readings on hardware
- Tune voltage thresholds

### Milestone 7.5 — UI Integration

- Add battery status to HeaderBar
- Add power information to the dashboard
- Add charging and USB indicators