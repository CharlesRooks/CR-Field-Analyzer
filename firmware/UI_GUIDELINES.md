# SentinelOS UI Guidelines

## Design Principle

SentinelOS uses an AMOLED-first interface optimized for field use, readability, and low power consumption.

> **Observe. Analyze. Adapt.**

---

## Theme

| Element | Style |
|---------|-------|
| Background | Black |
| Primary Text | White |
| Accent | Digicel Red |
| Muted Text | Grey |
| Success | Green |
| Warning | Yellow |
| Error | Red |

---

## Screen Layout

Every screen follows the same structure:

```
+--------------------------------------+
| Header Bar                           |
+--------------------------------------+
|                                      |
|                                      |
|          Content Area                |
|                                      |
|                                      |
+--------------------------------------+
| Navigation Bar                       |
+--------------------------------------+
```

---

## Header Bar

Purpose:

- Identify the application.
- Later display battery, Wi-Fi, Bluetooth, time and status icons.

Current Display:

```
SentinelOS
```

---

## Content Area

The active screen owns the content area.

Examples:

- Dashboard
- Scan
- Tools
- Settings
- Diagnostics

---

## Navigation Bar

The Navigation Bar provides access to the application's primary screens.

Current Items:

```
Dashboard | Scan | Tools | Settings
```

---

## Typography

| UI Element | Font |
|------------|------|
| Header | Montserrat 20 |
| Screen Title | Montserrat 20 |
| Body | Default LVGL Font |
| Navigation | Default LVGL Font |

---

## Design Rules

1. Black backgrounds wherever practical.
2. Update existing controls instead of recreating them.
3. Minimize redraws.
4. Global UI belongs to the UI layer.
5. Screens only own their content.
6. Keep layouts simple and readable in outdoor environments.
7. Consistency takes precedence over decoration.