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

```
                SentinelOS
                     │
        ┌────────────┼─────────────┐
        │            │             │
      Core          UI         Screens
        │                         │
        │                         │
        └────────────┬────────────┘
                     │
                 Services
                     │
                     │
          Hardware Abstraction
                     │
                     │
          LilyGO Hardware Library
                     │
                     │
                ESP32-S3 Hardware
```

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

- Theme

Future Classes

- StatusBar
- NavigationBar
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

Examples:

- BatteryService
- WiFiService
- BluetoothService
- StorageService
- SensorService
- SystemService
- GPSService

---

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

```
firmware/

src/

    Core/

    UI/

    Screens/

    Services/

    Hardware/

include/

lib/

platformio.ini

CHANGELOG.md

ROADMAP.md

ARCHITECTURE.md
```

---

# Design Rules

1. `main.cpp` should remain minimal.

2. Screens never communicate directly with hardware.

3. Hardware never communicates directly with Screens.

4. Services provide information.

5. Core coordinates the application.

6. UI owns visual appearance.

7. Every feature has one owner.

8. Every new module should integrate without modifying existing modules whenever possible.

9. Commit every stable milestone.

10. Documentation evolves alongside the code.

---

# Milestone Roadmap

| Milestone | Codename | Status |
|------------|----------|--------|
| Milestone 1 | **Foundation** | ✅ Complete |
| Milestone 2 | **Beacon** | ✅ Complete |
| Milestone 3 | **Awareness** | ⏳ In Progress |
| Milestone 4 | **Navigation** | Planned |
| Milestone 5 | **Recon** | Planned |
| Milestone 6 | **Insight** | Planned |
| Milestone 7 | **Expansion** | Planned |

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