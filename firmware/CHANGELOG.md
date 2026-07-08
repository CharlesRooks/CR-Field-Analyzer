# Changelog

All notable changes to SentinelOS / CR Field Analyzer will be documented here.

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

## v0.3.0 – Navigation Alpha

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