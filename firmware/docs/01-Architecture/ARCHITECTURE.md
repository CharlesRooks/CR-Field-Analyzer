# SentinelOS Architecture

> # Observe. Analyze. Adapt.
>
> *The guiding philosophy behind SentinelOS.*

---

# Vision

SentinelOS is a modular embedded operating system designed to power the **CR Field Analyzer** and future generations of portable field engineering tools.

Rather than being developed as a single-purpose Wi-Fi scanner, SentinelOS provides a common software platform capable of supporting multiple field analysis applications through a consistent, modular architecture.

Every design decision should support one or more of the following goals:

- Observe the environment.
- Analyze collected information.
- Adapt through modular expansion.

---

# Project Identity

| Item | Value |
|------|-------|
| **Operating System** | SentinelOS |
| **Device** | CR Field Analyzer |
| **Architecture Motto** | **Observe. Analyze. Adapt.** |
| **Primary Theme** | AMOLED Dark |
| **Primary Accent Colour** | Digicel Red |
| **Display Philosophy** | Minimal Power Consumption |
| **Development Methodology** | Incremental Milestones |
| **Architecture Style** | Modular Embedded System |
| **Source Control Strategy** | Commit every stable milestone |

---

# Design Philosophy

SentinelOS is designed using production software engineering principles commonly found in commercial embedded systems.

The objective is to build software that remains understandable, maintainable and expandable over many years without requiring large architectural redesigns.

---

## Modularity

Every subsystem should have one clearly defined responsibility.

New functionality should be added as independent modules whenever practical.

Modules should have minimal knowledge of one another.

---

## Separation of Concerns

Each software layer performs one specific role.

```
Application

↓

Core

↓

Screens

↓

Services

↓

Hardware Abstraction

↓

LilyGO Hardware Library

↓

ESP32 Hardware
```

Higher layers should never directly manipulate lower-level hardware.

---

## Single Responsibility

Every class should have one reason to change.

Examples:

- DashboardScreen displays information.
- BatteryService provides battery information.
- Theme controls appearance.
- SentinelOS coordinates the application.

---

## Hardware Abstraction

Hardware-specific code should never appear inside application screens.

Instead:

```
Dashboard

↓

BatteryService

↓

Battery Hardware

↓

LilyGO Library
```

This allows hardware to evolve without requiring application changes.

---

## Low Power First

SentinelOS is designed for battery-powered field operation.

Power efficiency should influence every design decision.

Guidelines:

- AMOLED-first design
- Black backgrounds whenever practical
- Minimal screen redraws
- Event-driven updates
- Efficient CPU usage
- Minimize wireless activity when idle

---

## Production Quality

SentinelOS follows commercial firmware development practices.

Every feature should include:

- Stable commits
- Documentation
- Architecture review
- Incremental testing
- Clear ownership

---

## Extensibility

SentinelOS is intended to become a platform rather than a single application.

Future capabilities include:

- Wi-Fi Analysis
- Bluetooth Analysis
- Environmental Monitoring
- GPS
- Storage Analysis
- Packet Capture
- Diagnostics
- Future sensor modules

The architecture should support these additions without major redesign.

---

## User Experience

The finished device should behave like a professional field instrument.

The user should never feel they are interacting with a development board.

Characteristics:

- Fast startup
- Responsive interface
- Professional appearance
- Clear navigation
- Reliable operation
- Predictable behaviour

---

# Software Architecture

                    SentinelOS
                         │
          ┌──────────────┼──────────────┐
          │              │              │
        Core         Managers          UI
          │              │              │
          │              │         Views / Widgets
          │              │
          └──────────────┼──────────────┘
                         │
                      Services
                         │
               Hardware Abstraction
                         │
              LilyGO Hardware Library
                         │
                  ESP32-S3 Hardware

---

# Layer Responsibilities

## Core

Responsible for application lifecycle.

Responsibilities:

- Startup
- Initialization
- State Machine
- Main Update Loop
- Screen Coordination

Current Classes

- SentinelOS
- MessageBus
- MessageTypes
- InputEvent

---

## UI

Responsible for appearance.

Responsibilities:

- Themes
- Colours
- Fonts
- Icons
- Navigation
- Status Bar
- Dialogs

Current Classes

- ApplicationFrame
- Theme
- HeaderBar
- NavigationBar
- StatusTile
- GridLayout
- SystemDashboardView

Future Classes

- DialogManager
- IconLibrary

---

## Screens

Responsible for user interaction.

Current Screens

- SplashScreen
- DashboardScreen

Future Screens

- ScanScreen
- BluetoothScreen
- DiagnosticsScreen
- ToolsScreen
- SettingsScreen
- AboutScreen

---

## Services

Services provide information to the application.

Screens request information from Services.

Services communicate with Hardware.

Current Services

- SystemService
- PowerService
- SleepService
- DisplayService

Planned Services

- WiFiService
- BluetoothService
- SensorService
- StorageService
- GPSService

---


## Managers

Managers coordinate behavior between services.

Managers do not communicate directly with hardware.

Examples:

Current Managers

- NavigationManager
- InputManager
- PowerManager

Responsibilities:

- Coordinate services.
- Maintain application state.
- Enforce system policies.
- Orchestrate user interaction.

## Hardware

Responsible for direct hardware interaction.

Examples:

- Display
- Touch
- Battery
- SD Card
- RTC
- Sensors

---

# Current Project Structure

```text
firmware/
├── src/
│   ├── Core/
│   │   ├── Messaging/
│   │   ├── InputEvent.h
│   │   ├── Page.h
│   │   ├── ScreenID.h
│   │   └── SentinelOS.*
│   ├── Managers/
│   ├── Screens/
│   ├── Services/
│   └── UI/
│       ├── Views/
│       └── Widgets/
├── docs/
│   ├── 01-Architecture/
│   ├── 02-Development/
│   ├── 03-UI/
│   └── 04-Services/
├── include/
├── lib/
└── platformio.ini
```

---


# Event and Messaging Architecture

SentinelOS uses a lightweight synchronous MessageBus to decouple event producers from event consumers.

```text
Touch Hardware
      │
      ▼
InputManager
      ├── UserActivity ───────────────► PowerManager
      │
      └── InputEvent ────────────────► NavigationManager
                                             │
                                             └── NavigationChanged
                                                        │
                                                        ▼
                                                    SentinelOS
                                                        │
                                                        ▼
                                                ApplicationFrame
```

The BOOT button is owned by `SentinelOS`. A button press publishes `UserActivity`, while the two-second long-press currently invokes `SleepService` directly.

## Active Message Flows

| Message | Publisher | Subscriber | Purpose |
|---------|-----------|------------|---------|
| `UserActivity` | `InputManager`; `SentinelOS` for BOOT-button activity | `PowerManager` | Reset idle time and restore the display |
| `InputEvent` | `InputManager` | `NavigationManager` | Carry `SwipeLeft`, `SwipeRight`, or `Tap` |
| `NavigationChanged` | `NavigationManager` | `SentinelOS` | Synchronize `ApplicationFrame` with the active `ScreenID` |

Power, display, sleep, application, Wi-Fi, and notification message types are reserved for future event flows.

## MessageBus Characteristics

- Fixed capacity of 16 subscriptions.
- Static function-pointer handlers.
- Synchronous dispatch: handlers run before `Publish()` returns.
- Nested publication is supported and already used by navigation.
- Messages are not queued, retained, or replayed.
- Duplicate subscriptions for the same type and handler are ignored as successful.
- `MessageType::None`, null handlers, and registrations beyond capacity are rejected.
- Callers must check subscription results and log failures.
- Handlers must be short and non-blocking.
- Handlers must not add or alter subscriptions during dispatch.

## Dependency Direction

Shared event definitions belong in Core rather than inside a manager.

```text
Core/InputEvent.h
      ▲             ▲
      │             │
InputManager   MessageTypes
```

This prevents the messaging layer from depending on `InputManager` and keeps event contracts independent from event producers.

---


# Power Management Architecture

SentinelOS separates power telemetry, policy, display capability, and sleep capability.

```text
PowerService ──cached power state──► PowerManager
                                        │
                           ┌────────────┴────────────┐
                           ▼                         ▼
                    DisplayService              SleepService
                  brightness/on/off          deep sleep/wake
```

`PowerService` owns PMU access and cached power telemetry. `PowerManager` owns USB and battery policy selection, idle timing, the display power-state machine, automatic deep-sleep decisions, and power-source transition handling. `DisplayService` owns brightness and display on/off capability. `SleepService` owns deep sleep and wake-source configuration.

The default battery sequence is:

```text
30 seconds idle  → Display dims
60 seconds idle  → Display turns off
120 seconds idle → Deep sleep
```

The USB policy disables automatic dimming, display-off, and deep sleep.

A power-source transition resets the idle timer and returns the display to its normal active brightness. This prevents a dark display after connecting USB and prevents immediate dimming after disconnecting USB.

Normal brightness and temporary dim brightness are tracked separately so display-off does not overwrite the brightness level that should be restored.

Detailed behaviour is documented in:

- `docs/01-Architecture/POWER_ARCHITECTURE.md`
- `docs/01-Architecture/POWER_POLICY.md`
- `docs/04-Services/POWER_SERVICE.md`

---

# Design Rules

1. `main.cpp` should remain minimal.

2. Screens never communicate directly with hardware.

3. Hardware never communicates directly with Screens.

4. Services own hardware capabilities.

5. Managers coordinate services.

6. Services do not coordinate one another.

7. UI consumes cached service data.

8. Hardware is accessed only through services.
   
9.  Managers coordinate Services.
    
10. Services never coordinate Managers.
    
11. Services do not directly coordinate one another.
    
12. Managers own policy.
    
13. Services own capability.

14. Event contracts belong in Core and must not depend on Managers.

15. Components publish events rather than directly invoking another component's internal behaviour when an event contract exists.

16. Message handlers must remain short, non-blocking, and safe for synchronous nested dispatch.

---

# Milestone Roadmap

Milestone 1  Hardware Bring-up
Milestone 2  Core UI Framework
Milestone 3  Navigation Framework
Milestone 4  Adaptive UI Framework
Milestone 5  Power Management Foundation
Milestone 6  Power Management Framework
Milestone 7  Power Management and Sleep
Milestone 8  MessageBus and Event Architecture
Milestone 9  Automatic Power Management

---

# Long-Term Objective

SentinelOS will evolve into a modular field engineering platform capable of supporting multiple analysis disciplines from a single handheld device.

The architecture should remain stable while capabilities continue to expand.

Every future feature should answer three questions before implementation:

1. **Where does it belong?**
2. **Who owns it?**
3. **How does it communicate?**

If these questions can be answered clearly, the feature is likely being implemented within the intended architecture.

---

> **Observe. Analyze. Adapt.**
>
> *One platform. Unlimited field tools.*
