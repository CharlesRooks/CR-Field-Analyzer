# SentinelOS

> **Observe. Analyze. Adapt.**

A modular embedded operating system for professional handheld field engineering tools.

---

## Overview

SentinelOS is an embedded application framework built for portable engineering instruments based on the ESP32 platform.

Rather than developing isolated applications, SentinelOS provides a reusable architecture that enables multiple field engineering tools to share a common operating environment.

The first device built on SentinelOS is the **CR Field Analyzer**.

---

## Design Goals

SentinelOS is built around five core principles:

- Modular Architecture
- Hardware Abstraction
- Low Power Operation
- Professional User Experience
- Long-Term Maintainability

Every component should be reusable, documented, and independently testable.

---

## Philosophy

> **Observe. Analyze. Adapt.**

SentinelOS is designed to help engineers observe their environment, analyze collected information, and adapt to changing technologies through modular expansion.

---

# Current Features

- ESP32-S3 Platform
- AMOLED User Interface
- LVGL Graphics Framework
- Page Framework
- Navigation Framework
- System Services
- Title Bar Widget
- Navigation Bar Widget
- Modular Screen Architecture

---

# Planned Features

## Navigation

- Swipe Gesture Navigation
- Animated Page Transitions
- Navigation Indicators

## Wireless

- Wi-Fi Scanner
- Bluetooth Scanner
- BLE Beacon Scanner
- Spectrum Analysis

## Sensors

- Battery Monitoring
- Environmental Sensors
- GPS
- Storage Monitoring

## Diagnostics

- Hardware Information
- Network Diagnostics
- System Diagnostics

---

# Architecture

SentinelOS follows a layered architecture.

```
Application
        │
        ▼
SentinelOS
        │
        ▼
Managers
        │
        ▼
Pages
        │
        ▼
Widgets
        │
        ▼
Services
        │
        ▼
Hardware
        │
        ▼
ESP32 Platform
```

---

# Repository Structure

```
firmware/
│
├── src/
│   ├── Core/
│   ├── Managers/
│   ├── Services/
│   ├── Screens/
│   ├── UI/
│   └── Hardware/
│
├── include/
├── lib/
└── platformio.ini
```

---

# Documentation

| Document | Description |
|----------|-------------|
| ARCHITECTURE.md | Software architecture |
| UI_GUIDELINES.md | User interface standards |
| API_REFERENCE.md | Public APIs |
| ROADMAP.md | Planned development |
| CHANGELOG.md | Version history |

---

# Development Methodology

SentinelOS follows an incremental development model.

Each milestone includes:

- Working firmware
- Documentation updates
- Stable Git commit
- Hardware validation

The project prioritizes small, tested improvements over large feature drops.

---

# Current Device

## CR Field Analyzer

The CR Field Analyzer is the reference hardware platform for SentinelOS.

Planned capabilities include:

- Wi-Fi Analysis
- Bluetooth Analysis
- Environmental Monitoring
- GPS
- Diagnostics
- Expandable modules

---

# Long-Term Vision

SentinelOS is intended to become a reusable embedded framework capable of powering multiple generations of professional handheld engineering tools.

The CR Field Analyzer is the first implementation of that vision.

---

> **Observe. Analyze. Adapt.**