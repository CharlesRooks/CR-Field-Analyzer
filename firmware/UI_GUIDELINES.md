# SentinelOS UI Guidelines

> **Observe. Analyze. Adapt.**
>
> *Every pixel should either convey information or improve usability.*

---

# Purpose

This document defines the visual language and interaction model used throughout SentinelOS.

Its purpose is to ensure every screen, widget, and future application maintains a consistent user experience while remaining optimized for professional field use.

---

# User Experience Philosophy

SentinelOS is not intended to resemble a smartphone application.

It is designed to function as a professional field engineering instrument.

The interface should prioritize:

- Readability
- Speed
- Reliability
- Consistency
- Low power consumption

Visual decoration should always remain secondary to usability.

---

# Design Principles

## Information First

Every visible element should communicate useful information or improve usability.

Avoid unnecessary decoration.

---

## Function Over Form

Visual appearance should never reduce readability or increase interaction complexity.

Simple interfaces are preferred over visually complex ones.

---

## Gesture First

Primary navigation should use natural gestures rather than precision touch targets.

Horizontal swipe gestures are the preferred method of moving between application pages.

Buttons should only be used where gestures are not appropriate.

---

## One-Handed Operation

The device should remain usable while being held in one hand.

Primary interactions should not require high touch accuracy.

The interface should remain usable while:

- Walking
- Standing
- Wearing light gloves
- Working outdoors

---

## AMOLED First

SentinelOS is designed specifically for AMOLED displays.

Therefore:

- Black backgrounds are preferred.
- Bright pixels should be minimized.
- Large white areas should be avoided.
- Animations should be subtle.

---

# Theme

| Element | Style |
|----------|-------|
| Background | Black |
| Primary Text | White |
| Secondary Text | Grey |
| Accent | Digicel Red |
| Success | Green |
| Warning | Yellow |
| Error | Red |

---

# Screen Layout

Every application page follows the same structure.

```
+------------------------------------------------+
| Title Bar                                      |
+------------------------------------------------+
|                                                |
|                                                |
|             Content Area                       |
|                                                |
|                                                |
+------------------------------------------------+
| Navigation Bar                                 |
+------------------------------------------------+
```

Only the **Content Area** belongs to the page itself.

Everything else is provided by the UI framework.

---

# Title Bar

The Title Bar is a persistent UI widget.

Responsibilities include:

- Application title
- Current page title
- Battery status (future)
- Wi-Fi status (future)
- Bluetooth status (future)
- Time (future)

Example:

```
SentinelOS                       🔋 97%   📶
```

The Title Bar should remain fixed during page transitions.

---

# Content Area

Each page owns only its content area.

Pages should never create:

- Title bars
- Navigation bars
- Global status indicators

Pages should focus exclusively on presenting information relevant to their function.

---

# Navigation Philosophy

Navigation is managed by the NavigationManager.

The Navigation Bar displays the user's current location but is **not** the primary navigation mechanism.

Primary navigation uses horizontal swipe gestures.

Navigation order:

```
Dashboard → Scan → Tools → Settings
```

Example:

```
● Dashboard   ○ Scan   ○ Tools   ○ Settings
```

The active page is indicated by a filled marker.

Future versions may animate transitions while preserving this interaction model.

---

# Typography

| UI Element | Font |
|------------|------|
| Title Bar | Montserrat 20 |
| Screen Title | Montserrat 20 |
| Section Heading | Montserrat 18 |
| Body Text | Default LVGL Font |
| Navigation Bar | Default LVGL Font |

---

# Layout Guidelines

Maintain consistent spacing throughout the application.

Recommended margins:

- Left: 10 px
- Right: 10 px
- Top: 10 px
- Bottom: 10 px

Widgets should align to a predictable visual grid.

---

# Animation Guidelines

Animations should communicate changes in application state.

Avoid decorative animations.

Examples of acceptable animations:

- Page slide transitions
- Progress indicators
- Loading animations
- Status updates

Animations should remain short and unobtrusive.

---

# UI Component Responsibilities

## Pages

Pages own only their content.

---

## Widgets

Widgets own reusable interface elements.

Examples:

- TitleBar
- NavigationBar
- StatusBar (future)
- Dialogs (future)

---

## Managers

Managers coordinate application behavior.

Examples:

- NavigationManager

---

## Services

Services provide information.

Examples:

- SystemService
- BatteryService
- WiFiService
- BluetoothService

---

## Hardware

Hardware layers abstract physical devices.

Pages should never communicate directly with hardware.

---

# Design Rules

1. Every pixel should either convey information or improve usability.
2. Screens own only their content.
3. Widgets own shared interface elements.
4. Managers coordinate application flow.
5. Services provide information.
6. Hardware abstracts physical devices.
7. Prefer gestures over precision touch targets.
8. Minimize redraws whenever possible.
9. Update existing widgets instead of recreating them.
10. Consistency is more important than decoration.

---

# Long-Term Vision

SentinelOS should feel like a professional handheld engineering instrument.

The interface should be immediately understandable to new users while remaining efficient for experienced engineers.

Every future feature should integrate naturally into the existing design system rather than introducing a new visual style or interaction model.

When in doubt, prefer simplicity over complexity.