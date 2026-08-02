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

## Core InputEvent

Header

```text
src/Core/InputEvent.h
```

Purpose

Defines the normalized input events shared by `InputManager`, `MessageTypes`, and event subscribers without creating a dependency on a manager implementation.

### Values

| Value | Description |
|-------|-------------|
| `None` | No completed input gesture |
| `SwipeLeft` | Completed horizontal swipe to the left |
| `SwipeRight` | Completed horizontal swipe to the right |
| `Tap` | Completed tap gesture |

---

## MessageBus

Header

```text
src/Core/Messaging/MessageBus.h
```

Purpose

Provides lightweight synchronous publish/subscribe messaging between SentinelOS components.

### Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `Begin()` | `void` | Clears all subscriptions and resets the bus |
| `Subscribe(type, handler)` | `bool` | Registers a handler for a message type |
| `Publish(message)` | `void` | Immediately invokes matching subscribers |

### Subscription Behaviour

- The bus supports a maximum of 16 subscriptions.
- `MessageType::None` and null handlers are rejected.
- Re-registering the same message type and handler is treated as success without creating a duplicate subscription.
- `Subscribe()` returns `false` when registration is invalid or capacity is exhausted.
- Subscription failures must be logged by the calling component.

### Dispatch Behaviour

- Dispatch is synchronous.
- `Publish()` does not queue, retain, or copy messages for later processing.
- Handlers execute before `Publish()` returns.
- A handler may publish another message, creating nested synchronous dispatch.
- Handlers must remain short, non-blocking, and must not modify subscriptions during dispatch.

### Active Messages

| Message | Publisher | Subscriber | Payload |
|---------|-----------|------------|---------|
| `UserActivity` | `InputManager`, `SentinelOS` for the BOOT button | `PowerManager` | None |
| `InputEvent` | `InputManager` | `NavigationManager` | `InputEvent` |
| `NavigationChanged` | `NavigationManager` | `SentinelOS` | `ScreenID` |

The remaining message types in `MessageTypes.h` are reserved for future power, display, sleep, application, Wi-Fi, and notification event flows.

---

## InputManager

Header

```text
src/Managers/InputManager.h
```

Purpose

Normalizes raw touch-controller data into high-level input events and publishes completed gestures through `MessageBus`.

### Methods

| Method | Purpose |
|--------|---------|
| `Begin()` | Attach the AMOLED device used for touch input |
| `Update()` | Poll touch input, classify completed gestures, and publish events |

---

## NavigationManager

Header

```text
src/Managers/NavigationManager.h
```

Purpose

Owns the active screen, responds to published input events, and publishes navigation changes.

### Methods

| Method | Purpose |
|--------|---------|
| `Begin()` | Attach the frame content area and subscribe to input events |
| `Show()` | Display a specific screen and publish `NavigationChanged` |
| `Next()` | Move to the next screen |
| `Previous()` | Move to the previous screen |
| `Update()` | Refresh the active page |
| `Current()` | Return the active `ScreenID` |

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

- WiFiService
- BluetoothService
- StorageService
- SensorService
- DiagnosticsService

Planned Managers

- ModuleManager

Planned Widgets

- DialogManager
- IconLibrary

---

# API Stability

The API follows semantic versioning.

Public APIs should remain backward compatible whenever practical.

Breaking changes should be documented in CHANGELOG.md.