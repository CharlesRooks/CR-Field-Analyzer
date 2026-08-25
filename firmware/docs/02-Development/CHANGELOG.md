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

### Milestone 10.24F — Existing Survey Point Markers

#### Added
- Added persistent Survey Point markers to the Floor Plan viewer.
- Added marker positioning from stored normalized 0–10000 Floor Plan coordinates.
- Added Survey Point ID labels for mapped Points.
- Added enhanced highlighting for the currently selected saved Survey Point.
- Added selected Survey Point name display.
- Added automatic suppression of non-selected Point labels on dense Floor Plans.
- Added navigation control to suspend global page-swipe gestures while Measurement Setup is active.

#### Changed
- Only Survey Points belonging to the active Site Survey and opened Floor Plan are rendered.
- Survey Point markers move with the Floor Plan during crosshair placement.
- The fixed placement crosshair and coordinate display remain above Survey Point markers.
- Marker labels now use the SentinelOS configured LVGL font instead of requiring an additional Montserrat font.
- Global left/right page navigation is temporarily disabled while Measurement Setup is open.
- Global page navigation is restored when Measurement Setup closes.

#### Hardened
- Floor Plan dragging can no longer trigger hidden application page navigation beneath Measurement Setup.
- Starting a Wi-Fi measurement after Floor Plan placement now returns to the Wi-Fi Scan screen as intended.
- Survey Points belonging to other Site Surveys or Floor Plans are excluded from the active map overlay.
- Marker positions remain derived from persistent Survey Point coordinates after reopening and reboot.

#### Verified
- Firmware compiled successfully.
- Existing mapped Survey Points rendered successfully on the Floor Plan.
- Survey Points from other Floor Plans and Site Surveys were excluded.
- The selected saved Survey Point was visually distinct from other markers.
- Survey Point markers moved correctly with the Floor Plan during placement.
- The fixed crosshair remained above the marker layer.
- Moving and saving a Survey Point updated its displayed marker position.
- Marker positions remained correct after reopening the Floor Plan.
- Marker positions remained correct after reboot.
- Floor Plan dragging did not trigger hidden page navigation.
- Starting a measurement after Floor Plan placement returned correctly to the Wi-Fi Scan screen.
- Wi-Fi measurement execution continued normally after Floor Plan placement.

### Milestone 10.24E — Crosshair Survey Point Placement

#### Added
- Added interactive Survey Point placement on registered Floor Plans.
- Added fixed-center crosshair positioning workflow.
- Added drag-to-position Floor Plan interaction beneath the fixed crosshair.
- Added normalized 0–10000 X/Y coordinate calculation from Floor Plan position.
- Added live X/Y percentage display during placement.
- Added support for assigning Floor Plan coordinates to existing persistent Survey Points.
- Added pending Floor Plan placement for newly created Survey Points.
- Added reopening of mapped Survey Points at their previously saved Floor Plan position.

#### Changed
- Floor Plan movement is constrained so the placement crosshair cannot move outside the actual image bounds.
- Saving an existing Survey Point position updates its persistent Floor Plan coordinates immediately.
- Saving placement now returns directly to Measurement Setup.
- Floor Plan workflow exit control is labelled `Back to Setup`.
- Existing mapped Points open directly at their stored location instead of defaulting to the center of the Floor Plan.
- Placement coordinate text is explicitly rendered in white for visibility over Floor Plan imagery.

#### Hardened
- Cancelling placement leaves the previously stored Survey Point position unchanged.
- New Survey Point coordinates remain pending until the Point is persistently created.
- Reopening a mapped Point restores its stored Floor Plan ID and normalized coordinates.
- Placement state is cleared appropriately when leaving the Floor Plan workflow.

#### Verified
- Firmware compiled successfully.
- Crosshair remained fixed while the Floor Plan moved underneath it.
- Floor Plan movement remained constrained to valid image bounds.
- Live X/Y percentages were visible and updated during movement.
- Existing Survey Point placement saved successfully.
- Saved Survey Point reopened at its stored Floor Plan position.
- New Survey Point placement completed successfully.
- Save returned directly to Measurement Setup.
- Cancel preserved the previously stored Survey Point position.
- Back to Setup exited the Floor Plan workflow correctly.
- Saved Floor Plan positions remained available after reboot.

### Milestone 10.24D — Floor Plan Viewer

#### Added
- Added full-screen Floor Plan viewer for registered Site Survey Floor Plans.
- Added Floor Plan image rendering service.
- Added PNG Floor Plan decoding support.
- Added JPEG/JPG Floor Plan decoding support.
- Added standard uncompressed 24-bit and 32-bit BMP rendering support.
- Added PSRAM-backed Floor Plan rendering buffer.
- Added aspect-ratio-preserving Fit rendering.
- Added navigation from the registered Floor Plan catalog into the viewer.
- Added Back navigation from the viewer to the Floor Plan catalog.

#### Changed
- Registered Floor Plans in Measurement Setup can now be opened directly for viewing.
- Floor Plan images are scaled to fit the available AMOLED viewport without distortion.
- Small Floor Plan images are not unnecessarily upscaled.
- Floor Plan transfer guidance now references Tools → USB Transfer instead of the previous read-only USB storage workflow.
- JPEGDEC is sourced directly from the upstream GitHub release because the requested PlatformIO Registry package version was unavailable.

#### Hardened
- Floor Plan viewer reports image decode failures instead of failing silently.
- Rendering uses PSRAM for the image canvas to reduce internal RAM pressure.
- Viewer resources are released when leaving the Floor Plan screen.
- Image dimensions and aspect ratio are preserved during rendering.

#### Verified
- Firmware compiled successfully.
- PNG and JPEG decoding dependencies compiled successfully.
- Firmware used approximately 40% of available RAM and 21% of configured application flash.
- Existing registered Floor Plan opened successfully on hardware.
- Floor Plan image rendered successfully on the AMOLED display.
- Fit-to-screen rendering displayed the complete Floor Plan.
- Image aspect ratio was preserved.
- Viewer navigation remained stable.
- Back navigation returned successfully to the Floor Plan catalog.
- Floor Plan could be reopened successfully.
- Registered Floor Plan remained available and viewable after reboot.

### Milestone 10.24C — Floor Plan Assignment & Persistent Survey Point Coordinates

#### Added
- Added persistent Floor Plan references to Survey Point records.
- Added normalized Survey Point map coordinates using a 0–10000 coordinate space.
- Added `floor_plan_id`, `map_x`, and `map_y` fields to persistent Survey Points.
- Added Survey Point storage format version 2.
- Added validation that a referenced Floor Plan belongs to the same Site Survey as the Survey Point.
- Added coordinate-range validation for mapped Survey Points.
- Added protected Survey Point record replacement with backup recovery for future coordinate updates.

#### Changed
- New Survey Points are now stored using format version 2.
- Unmapped Survey Points use `floor_plan_id=0`, `map_x=0`, and `map_y=0`.
- Existing format version 1 Survey Points remain fully backward compatible and load as unmapped Points.
- Persistent Survey Point records are now ready to support Floor Plan positioning without changing measurement-session identity.

#### Verified
- Firmware compiled successfully.
- Existing version 1 Survey Points remained selectable and reusable.
- Existing version 1 Points could be used for new measurements without error.
- New Survey Point records were written successfully using format version 2.
- New version 2 Point records contained `floor_plan_id`, `map_x`, and `map_y`.
- Unmapped version 2 Points correctly stored zero-valued Floor Plan and coordinate fields.
- Version 1 and version 2 Survey Points coexisted successfully after reboot.
- Existing Floor Plan catalog remained intact.
- Existing Wi-Fi History sessions continued to load normally.

### Milestone 10.24B — Floor Plan Import, Catalog & Safe USB Transfer

#### Added
- Added Floor Plan image import workflow for Site Surveys.
- Added discovery of JPG, JPEG, PNG, and BMP Floor Plan images.
- Added Floor Plan image dimension detection and validation.
- Added persistent Floor Plan catalog filtered by parent Site Survey.
- Added Floor Plan selection and registration from Measurement Setup.
- Added `/sentinel/import/floorplans/` staging directory for incoming Floor Plan images.
- Added automatic registration and storage of imported images under `/sentinel/floorplans/images/`.
- Added safe read/write USB Transfer Mode for PC-to-SentinelOS file transfer.
- Added USB Mass Storage write support using raw SD sector writes.
- Added USB transfer read, write, and failed-write diagnostics.
- Added USB Transfer controls to the Tools screen.
- Added 800 × 480 Floor Plan hardware test image.

#### Changed
- USB Mass Storage is now read/write only while explicit USB Transfer Mode is active.
- SentinelOS relinquishes filesystem ownership while the host controls the SD card.
- SentinelOS storage operations remain blocked for the duration of USB Transfer Mode.
- The device requires a restart after the host releases the writable SD card so the filesystem is remounted and all catalogs are rebuilt from clean state.
- Floor Plan import directory enumeration was hardened so discovered files can be reopened and validated reliably.
- Floor Plan images can be transferred without physically removing the microSD card.

#### Hardened
- SentinelOS and the USB host are prevented from simultaneously modifying the FAT filesystem.
- Site Survey, Survey Point, measurement-session, and Floor Plan writes remain blocked while USB Transfer Mode owns the SD card.
- Host write failures are counted and exposed through USB Transfer diagnostics.
- Floor Plan records whose backing image is unavailable are rejected during catalog recovery.
- Writable USB storage does not return filesystem control to SentinelOS until a clean restart.

#### Verified
- Firmware compiled successfully.
- Writable USB Mass Storage mounted successfully in Windows.
- Files could be copied from Windows to the SentinelOS SD card.
- Transferred files retained their expected non-zero size.
- Windows safe eject completed successfully.
- SentinelOS retained exclusive-storage protection until restart.
- Device restarted successfully after USB transfer.
- Floor Plan import scanner detected the transferred test image.
- Imported Floor Plan could be registered to an existing Site Survey.
- Registered Floor Plan remained available after reboot.
- Existing Site Surveys and Survey Points remained intact.
- Physical microSD removal is no longer required for routine Floor Plan transfer.

### Milestone 10.24A — Floor Plan Persistent Storage Foundation

#### Added
- Added persistent Floor Plan data model.
- Added globally unique Floor Plan IDs.
- Added parent Site Survey references for Floor Plans.
- Added CRC32-protected Floor Plan metadata records.
- Added persistent Floor Plan catalog and startup recovery.
- Added `/sentinel/floorplans/` storage directory.
- Added `/sentinel/floorplans/images/` image storage directory.
- Added validation that Floor Plans belong to valid Site Surveys.
- Added monotonic Floor Plan ID allocation with protection against ID reuse.

#### Verified
- Firmware compiled successfully.
- Device booted successfully on hardware.
- Floor Plan storage directories were created automatically.
- Existing Site Survey records remained intact.
- Existing Survey Point records remained intact.
- USB Mass Storage continued to operate normally.

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
