# Changelog

All notable changes to SentinelOS / CR Field Analyzer will be documented here.

## [Unreleased]

### Added
- MessageBus framework for synchronous publish/subscribe communication.
- `UserActivity`, `InputEvent`, and `NavigationChanged` event flows.
- Core `InputEvent` contract shared by input and messaging components.
- Navigation-change messages carrying the active `ScreenID`.
- Subscription diagnostics for SentinelOS, NavigationManager, and PowerManager.
- Automatic battery deep sleep after the configured idle timeout.
- Power-source transition handling for USB and battery policies.
- Dedicated power-architecture documentation.

### Changed
- InputManager now publishes completed touch gestures and user-activity events.
- NavigationManager now consumes `InputEvent` messages and publishes `NavigationChanged`.
- PowerManager now receives user activity through MessageBus.
- SentinelOS now coordinates ApplicationFrame updates from navigation-change messages.
- `PowerManager::NotifyActivity()` is private to prevent direct event-path bypass.
- Message types not yet implemented are explicitly marked as reserved.
- PowerManager now enforces the complete battery sequence: dim, display-off, and deep sleep.
- Battery-to-USB and USB-to-battery transitions reset idle timing.
- Power-source transitions return the display to its normal active brightness.
- PowerService documentation now distinguishes telemetry ownership from policy, display, and sleep ownership.

### Fixed
- Normal display brightness being overwritten by the temporary dimmed brightness when the display turned off.
- Display waking at the dimmed brightness instead of the saved normal brightness.
- A dimmed or off display remaining dark after USB was connected.
- Immediate dimming risk after USB was disconnected following a long USB-powered session.

### Hardened
- Duplicate subscriptions for the same message type and handler are prevented.
- Invalid subscriptions using `MessageType::None` or null handlers are rejected.
- Subscription-capacity failures are surfaced through serial diagnostics.
- MessageBus initialization and serial diagnostic ordering were corrected.
- DisplayService now tracks temporary dimming separately from the saved normal brightness.

### Technical
- MessageBus supports 16 fixed subscriptions without dynamic allocation.
- Dispatch is synchronous and supports nested publication.
- Messages are not queued, retained, or replayed.
- Handlers are required to remain short and non-blocking.
- The messaging layer no longer depends on InputManager.
- USB policy disables automatic dimming, display-off, and deep sleep.
- Battery policy defaults to 30-second dim, 60-second display-off, and 120-second deep sleep.
- PowerManager delegates display actions to DisplayService and deep sleep to SleepService.

### Verified
- Swipe-left and swipe-right navigation through MessageBus.
- Tap activity without unintended navigation.
- Display dimming, display-off, and touch wake behaviour.
- Full-brightness restoration from both dimmed and off states.
- BOOT-button activity and two-second manual deep-sleep entry.
- Automatic deep sleep while operating on battery.
- BOOT-button wake after automatic deep sleep.
- Battery-to-USB transition while dimmed.
- Battery-to-USB transition while the display was off.
- USB-to-battery transition with fresh battery-policy timing.
- No subscription errors during hardware regression testing.

## [0.5.0-alpha] - 2026-07-11

### Added
- PowerService architecture.
- PowerInfo data model.
- BQ25896 power-management integration.
- Battery voltage monitoring.
- Battery percentage estimation.
- USB power detection.
- Dynamic power-source identification.
- Power status tile on System Dashboard.
- Persistent power indicator in HeaderBar.
- Centralized power-status formatting.
- BQ25896 charging-state validation.
- Conditional charger enablement when USB and battery are present.
- Battery percentage smoothing.
- Piecewise LiPo voltage-to-percentage curve.
- Interpolated battery percentage estimation.
- SleepService framework.
- Manual deep-sleep support.
- BOOT-button deep-sleep wake support.

### Changed
- Renamed the Battery dashboard tile to Power.
- ApplicationFrame now supports recurring frame updates.
- HeaderBar now consumes live PowerService status.
- Battery percentage now compensates for elevated charging voltage.
- Battery percentage now preserves the last valid discharge estimate while charging.
- Charging status now uses validated BQ25896 charge states.
- Replaced linear battery percentage estimation with a nonlinear LiPo discharge curve.
- Power status formatting centralized through PowerService.

### Fixed

- Battery percentage oscillation caused by LiPo voltage recovery.
- Incorrect battery percentage transitions between USB and battery operation.
- Immediate deep-sleep wake caused by entering sleep while the BOOT button was still pressed.

### Technical
- Power-management data is cached by PowerService.
- PMU polling is throttled to one-second intervals.
- UI components consume cached power data.
- Power status dynamically updates between USB and battery operation.
- Introduced a dedicated SleepService to separate sleep management from power monitoring.


## [0.4.0-alpha] - 2026-07-11

### Added
- StatusTile reusable UI widget.
- GridLayout adaptive layout system.
- SystemDashboardView.
- Reusable View architecture.
- Grid-based dashboard tile positioning.
- Horizontal dashboard scrolling.

### Changed
- Dashboard content migrated to SystemDashboardView.
- Dashboard status information migrated to reusable StatusTile widgets.
- UI layout responsibilities separated from page navigation.
- Dashboard architecture prepared for additional system services.

### Fixed
- Dashboard content overflow beyond the visible display area.
- Status tile overlap with dashboard content.
- Layout positioning issues on the AMOLED display.
- Dashboard tile accessibility through horizontal scrolling.

### Technical
- Introduced reusable View architecture.
- Added GridLayout-based component positioning.
- Separated DashboardScreen from dashboard presentation logic.
- Established reusable widget and layout patterns for future SentinelOS views.
  

## [0.3.0-alpha] - 2026-07-06

### Added
- InputManager framework
- Swipe gesture recognition
- Gesture-based page navigation
- Page content container architecture

### Changed
- NavigationManager now owns navigation state.
- Page framework now manages shared UI components.

### Fixed
- Root screen scrolling disabled.
- Improved touch gesture recognition for CST touch controller.
  

## [0.2.0-alpha] - 2026-07-04

### Added
- Modular SentinelOS architecture.
- Core `SentinelOS` controller.
- AMOLED-friendly black theme.
- Splash screen.
- Dynamic dashboard screen.
- Live uptime counter.
- Live heap display.
- LilyGO AMOLED + LVGL display stack.

### Verified
- ESP32-S3 firmware upload.
- USB serial monitor.
- 16 MB flash.
- 8 MB PSRAM.
- AMOLED display.
- Touch input.
- LVGL rendering.

## [0.1.0-alpha] - 2026-07-04

### Added
- Initial PlatformIO project.
- LilyGO T-Display S3 AMOLED board configuration.
- Initial board bring-up diagnostics.

### Verified
- ESP32-S3 CPU detection.
- Flash size detection.
- PSRAM enablement.
- USB CDC serial output.
