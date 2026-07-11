# Changelog

All notable changes to SentinelOS / CR Field Analyzer will be documented here.

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

### Changed
- Renamed the Battery dashboard tile to Power.
- ApplicationFrame now supports recurring frame updates.
- HeaderBar now consumes live PowerService status.

### Technical
- Power-management data is cached by PowerService.
- PMU polling is throttled to one-second intervals.
- UI components consume cached power data.
- Power status dynamically updates between USB and battery operation.


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