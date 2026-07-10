# SentinelOS API Reference

> **Observe. Analyze. Adapt.**

---

# Purpose

This document describes the public APIs exposed by SentinelOS.

Only documented public interfaces should be used by applications, screens, or future modules.

Implementation details may change over time, but documented APIs should remain stable whenever practical.

---

# API Design Philosophy

SentinelOS APIs should be:

- Simple
- Consistent
- Self-documenting
- Hardware independent
- Easy to extend

Application code should communicate with Services and Managers rather than directly with hardware.

---

# Naming Convention

Classes use PascalCase.

Example:

SystemService

Methods use PascalCase.

Example:

GetFreeHeapKB()

Boolean methods begin with:

- Is
- Has
- Can

Examples:

HasPSRAM()

CanSleep()

Method names should describe what they return.

Avoid ambiguous names.

Preferred:

GetFormattedUptime()

Avoid:

Read()

Update()

Check()

unless the context is obvious.

---

# Current Public APIs

## SystemService

Header

```
src/Services/System/SystemService.h
```

Purpose

Provides general system information.

### Methods

| Method | Returns | Description |
|---------|----------|-------------|
| GetFlashSizeMB() | uint32_t | Installed Flash memory |
| HasPSRAM() | bool | PSRAM availability |
| GetFreeHeapKB() | uint32_t | Free heap memory |
| GetUptimeSeconds() | uint32_t | System uptime in seconds |
| GetFormattedUptime() | String | Uptime formatted as HH:MM:SS |
| GetChipModel() | String | ESP32 model |
| GetCPUFrequencyMHz() | uint32_t | CPU frequency |

Example

```cpp
auto heap = SystemService::GetFreeHeapKB();

auto uptime = SystemService::GetFormattedUptime();
```

---

# NavigationManager

Purpose

Controls application page navigation.

(Currently under development.)

Planned methods

| Method | Purpose |
|---------|----------|
| Show() | Display page |
| Next() | Next page |
| Previous() | Previous page |
| Current() | Current page |

---

# Page

Purpose

Base class for all application pages.

Current methods

| Method | Purpose |
|---------|----------|
| Show() | Display page |
| Hide() | Hide page |
| Update() | Refresh page |
| CreateContent() | Build page contents |

---

# Future APIs

Planned Services

- BatteryService
- WiFiService
- BluetoothService
- StorageService
- SensorService
- DiagnosticsService

Planned Managers

- NavigationManager
- ModuleManager

Planned Widgets

- TitleBar
- NavigationBar
- StatusBar

---

# API Stability

The API follows semantic versioning.

Public APIs should remain backward compatible whenever practical.

Breaking changes should be documented in CHANGELOG.md.