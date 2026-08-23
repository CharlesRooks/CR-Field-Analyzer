# Changelog

All notable changes to SentinelOS / CR Field Analyzer will be documented here.

## [Unreleased]

### Added

* MessageBus framework for synchronous publish/subscribe communication.
* `UserActivity`, `InputEvent`, and `NavigationChanged` event flows.
* Core `InputEvent` contract shared by input and messaging components.
* Navigation-change messages carrying the active `ScreenID`.
* Subscription diagnostics for SentinelOS, NavigationManager, and PowerManager.
* Production TinyUSB composite USB architecture providing CDC serial and read-only SD Mass Storage.
* `UsbStorageService` production integration for read-only USB Mass Storage.
* `flash-monitor.ps1` development helper for ESP32-S3 firmware upload, application-port detection, and automatic serial monitoring.
* Saved Wi-Fi measurement sessions can now display the persisted per-BSSID network inventory in the History Networks view.
* Historical network rows use the average RSSI measured across the completed measurement session.
* History Networks now displays per-BSSID measurement details including observation count, RSSI range, and BSSID. 
* Historical network signal strength is explicitly identified as the average RSSI across the completed measurement session.
* Survey Point labels can now be assigned to Wi-Fi measurement sessions from the Scan screen.
* Added an on-screen Survey Point editor using the LVGL keyboard.
* Saved measurement-session format version 5 adds an optional site-survey point label.
* Saved History sessions display their Survey Point label when available.

### Changed

* InputManager now publishes completed touch gestures and user-activity events.
* NavigationManager now consumes `InputEvent` messages and publishes `NavigationChanged`.
* PowerManager now receives user activity through MessageBus.
* SentinelOS now coordinates ApplicationFrame updates from navigation-change messages.
* `PowerManager::NotifyActivity()` is now private to prevent direct event-path bypass.
* Message types not yet implemented are explicitly marked as reserved.
* Production LilyGO USB configuration now uses TinyUSB OTG mode with `ARDUINO_USB_MODE=0` and `ARDUINO_USB_CDC_ON_BOOT=1`.
* PlatformIO serial monitoring now asserts DTR with `monitor_dtr = 1` to support reliable TinyUSB CDC output.
* Development upload workflow now accounts for ESP32-S3 ROM-download and SentinelOS application USB re-enumeration.
* Wi-Fi History now supports both Networks and Channels views without leaving the selected saved session.
* Entering History preserves the currently selected Wi-Fi presentation.
* Saved-session status remains visible while reviewing either Networks or Channels.
* Saved network rows now provide expanded site-survey detail while preserving the compact live Networks presentation.
* Historical network entries show how many successful session scans observed each BSSID and the minimum-to-maximum RSSI range recorded during the session.
* Wi-Fi measurement summaries now retain the Survey Point label captured for the completed session.
* New saved sessions are written using storage format version 5 while maintaining compatibility with versions 1 through 4.
* Survey Point editing is disabled while an automatic measurement session is active.
* Survey Point editing now uses a full-screen overlay to provide substantially larger and more accurate touch targets on the AMOLED keyboard.
* The working Survey Point label is automatically cleared after a completed measurement summary captures it, preventing accidental reuse at the next physical survey location.

### Hardened

* Duplicate subscriptions for the same message type and handler are prevented.
* Invalid subscriptions using `MessageType::None` or null handlers are rejected.
* Subscription-capacity failures are surfaced through serial diagnostics.
* MessageBus initialization and serial diagnostic ordering were corrected.
* USB Mass Storage exposes the SD card to the host as read-only media.
* `flash-monitor.ps1` now allows additional time for the TinyUSB CDC interface to stabilize after application reset.
* CDC monitor startup re-detects the SentinelOS COM port and retries when Windows temporarily removes or re-enumerates the USB serial interface.
* Legacy version-1 through version-4 saved sessions continue to load without requiring Survey Point metadata.
* Version-5 session parsing validates and restores the persisted Survey Point field.
* Temporary storage round-trip verification now includes the Survey Point label.

### Technical

* MessageBus supports 16 fixed subscriptions without dynamic allocation.
* Dispatch is synchronous and supports nested publication.
* Messages are not queued, retained, or replayed.
* Handlers are required to remain short and non-blocking.
* The messaging layer no longer depends on InputManager.
* SentinelOS USB CDC and Mass Storage operate as interfaces of a single TinyUSB composite device.
* Firmware upload uses the ESP32-S3 ROM downloader, followed by a manual hardware reset to start the TinyUSB application.
* `flash-monitor.ps1` detects the application CDC interface after reset and launches PlatformIO monitoring automatically.
* PlatformIO monitor configuration is standardized at 115200 baud with DTR active and RTS inactive.

### Verified

* Swipe-left and swipe-right navigation through MessageBus.
* Tap activity without unintended navigation.
* Display dimming, display-off, and touch wake behaviour.
* BOOT-button activity and two-second deep-sleep entry.
* No subscription errors during hardware regression testing.
* Full SentinelOS boot sequence captured through TinyUSB CDC.
* Windows enumeration of CDC serial, read-only USB Mass Storage, and USB Composite Device.
* RTC detection, valid RTC time, system-clock synchronization, and UTC-04:00 time-zone configuration.
* SD card mounting, filesystem reporting, and saved measurement-session indexing.
* `UsbStorageService` read-only media initialization.
* USB power-policy detection.
* Wi-Fi service initialization and successful startup scan.
* Splash-to-Running application-state transition.
* End-to-end `flash-monitor.ps1` upload, manual-reset, CDC detection, and serial-monitor workflow.
* History Networks and Channels navigation on hardware.
* Older/Newer saved-session navigation in both History views.
* Correct display of version-4 saved network inventories.
* Graceful handling of legacy saved sessions without network inventory data.
* Historical BSSID, observation count, average RSSI, and RSSI range display on the LilyGO AMOLED.
* Expanded History network rows remain readable and usable on the physical display.
* Version-4 saved measurement sessions correctly expose detailed per-BSSID survey data.
* Legacy saved sessions without network inventory continue to fail gracefully with an explanatory message.
* Survey Point control fits the LilyGO AMOLED Scan screen without materially reducing usable results space.
* On-screen Survey Point keyboard is usable on the physical device.
* Survey Point labels persist through completed three-scan measurement sessions.
* Version-5 Survey Point labels are written to SD storage and restored after reboot.
* Survey Point labels display correctly in both History Networks and Channels views.
* Existing version-4 saved sessions remain readable and continue to display normally without Survey Point labels.
* Full-screen Survey Point keyboard is comfortable and accurate for text entry on the physical LilyGO AMOLED display.
* Completed sessions retain their assigned Survey Point after the working label is cleared.
* After a completed measurement, the Scan screen correctly returns to `Point: Set survey location` for the next survey point.


### Milestone 10.22B — Site Survey Lifecycle & Reboot Recovery

#### Added
- Added persistent active Site Survey context using `/sentinel/surveys/active.txt`.
- Added CRC-protected active survey storage.
- Added active Site Survey restoration during SentinelOS startup.
- Added explicit `Close Survey` control to Measurement Setup.
- Added active survey cleanup when a Site Survey is closed.
- Added rollback handling to preserve the previous survey context if a survey switch fails.

#### Changed
- Active Site Surveys now survive device resets and power cycles.
- Survey Point remains intentionally transient and is not restored after reboot.
- Measurement Setup only displays `Close Survey` when a Site Survey is currently active.

#### Verified
- Active Site Survey restored successfully after physical reset.
- Survey Point remained blank after reboot.
- Close Survey removed the active survey context.
- Closed Site Survey did not return after reboot.
- Measurement Setup correctly reflected active and inactive survey states.

### Milestone 10.22A — Site Survey Measurement Setup

#### Added
- Added `SiteSurveyService` and `SiteSurveyManager` for long-lived Site Survey context.
- Added persistent Site Survey records under `/sentinel/surveys/`.
- Added independent Site Survey ID sequencing.
- Added Site Survey ID and name to Wi-Fi measurement summaries.
- Added measurement storage format v6 with parent Site Survey references.
- Added dedicated `MeasurementSetupScreen`.
- Added Site Survey and Survey Point entry before starting Wi-Fi measurements.
- Added full-screen text editors and keyboard workflow for survey data entry.
- Added automatic reuse of the active Site Survey across multiple Survey Points.
- Added automatic creation of a new Site Survey when the survey name changes.
- Added Site Survey and Survey Point information to Wi-Fi History.
- Added `flash.ps1` for fast firmware upload without opening the serial monitor.

#### Changed
- `New` now opens Measurement Setup before starting a three-scan measurement session.
- `Scan` now opens Measurement Setup before starting a single scan.
- Survey Point is cleared after each completed measurement while the active Site Survey is retained.
- Removed Site Survey and Survey Point controls from the Wi-Fi results area to preserve usable screen space.
- Existing `flash-monitor.ps1` remains available when serial diagnostics are required.

#### Verified
- Measurement Setup layout on the physical T-Display S3 AMOLED.
- Full-screen keyboard operation for Site Survey and Survey Point entry.
- Multiple Survey Points under the same Site Survey.
- Active Site Survey name retained between measurements.
- Survey Point cleared for each new measurement.
- Site Survey and Survey Point displayed correctly in both Networks and Channels History views.
- Fast firmware upload using `flash.ps1`.


### Milestone 10.22C — Site Survey Catalog & Resume Existing Survey

#### Added
- Added lightweight in-memory Site Survey catalog.
- Added startup enumeration and CRC32 validation of persisted Site Survey records.
- Added newest-first indexing of saved Site Surveys.
- Added automatic catalog synchronization when new Site Surveys are created.
- Added `ResumeSurvey()` support for reopening an existing Site Survey without creating a duplicate survey record.
- Added safe saved-survey switching with rollback to the previous active survey if resume fails.
- Added Saved Site Survey selector to Measurement Setup.

#### Changed
- Existing Site Surveys can now be deliberately reopened using their original Survey ID and creation timestamp.
- Manual Site Survey name entry continues to use the new-survey workflow.
- Manual editing after selecting a saved survey clears the stored selection identity to prevent accidental ID/name mismatches.
- Measurement Setup layout was compacted to accommodate the Saved Survey selector.
- Saved Survey selector uses a vertically scrollable list while keeping its controls accessible.
- Button heights were rebalanced for improved symmetry and use of the available display area.

#### Verified
- Saved Site Survey catalog populated successfully on hardware.
- Existing Site Survey could be selected and resumed.
- Resumed survey retained its original Survey ID.
- Resuming an existing survey did not create a duplicate permanent survey record.
- Resumed Site Survey survived physical reboot as the active survey.
- Survey Point remained transient.
- Saved Survey selector scrolling operated correctly.
- Measurement Setup and action controls fit fully on the display.


### Milestone 10.23B — Persistent Survey Point Registry & Reuse Workflow

#### Added
- Added persistent Survey Point records with globally unique Point IDs.
- Added parent Site Survey references to persisted Survey Points.
- Added CRC32-protected Survey Point storage under `/sentinel/survey_points/`.
- Added in-memory Survey Point catalog with startup recovery.
- Added monotonic Survey Point ID allocation with protection against ID reuse.
- Added validation of Survey Point parent Site Survey records.
- Added persistent Survey Point selection and reuse from Measurement Setup.
- Added manager coordination for creating new and preparing saved Survey Points.
- Added Survey Point ID support to the Wi-Fi measurement working context.
- Added Measurement Session format v7 with persistent `survey_point_id`.

#### Changed
- Survey Points are now persistent entities rather than free-text measurement labels only.
- Measurement records retain both Survey Point ID and human-readable Survey Point name.
- Saved Survey Points can only be reused within the Site Survey that owns them.
- Manual Point entry creates a new persistent Survey Point.
- Selecting a saved Point reuses its original Point ID.
- Working Survey Point identity is cleared after each completed measurement while the Site Survey remains active.
- Measurement session parser remains backward compatible with formats v1 through v6.

#### Verified
- New Survey Point created and persisted successfully on hardware.
- Saved Survey Point appeared correctly in the Point selector.
- Existing Survey Point could be reused for additional measurements.
- Reusing a saved Point did not create a duplicate Point entry.
- Multiple Survey Points could be created under the same Site Survey.
- Survey Point catalog survived physical reset.
- Active Site Survey restored successfully after reboot.
- Survey Point remained intentionally blank after reboot.
- Existing saved Survey Points remained selectable after reboot.
- New format v7 measurement sessions loaded successfully from History.
- Legacy v1-v6 measurement sessions continued to load successfully.

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
