# SentinelOS Roadmap

**Current Version:** v0.5.0 (Power Management and Core Event Architecture)

---

## ✅ v0.1.0 – Hardware Bring-up

- ESP32-S3 initialization
- AMOLED display
- Touch controller
- Power management IC
- Flash detection
- PSRAM detection

**Status:** Complete

---

## ✅ v0.2.0 – Core UI Framework

- Theme engine
- HeaderBar
- NavigationBar
- Page framework
- ApplicationFrame
- Screen management

**Status:** Complete

---

## ✅ v0.3.0 – Core Services

- SystemService
- Application state machine
- NavigationManager
- InputManager
- Live dashboard updates

**Status:** Complete

---

## ✅ v0.4.0 – Adaptive UI Framework

- StatusTile widget
- GridLayout
- SystemDashboardView
- Reusable View architecture

**Status:** Complete

---

### 🟨 v0.5.0 – Power Management

- ✔ PowerService architecture
- ✔ PowerInfo data model
- ✔ BQ25896 PMU integration
- ✔ Battery voltage monitoring
- ✔ Battery percentage estimation
- ✔ USB power detection
- ✔ Power source identification
- ✔ Live System Dashboard power status
- ✔ Persistent HeaderBar power indicator
- ✔ Charging-state validation
- ✔ Conditional charger enablement
- ✔ Charging-voltage percentage compensation
- ✔ Battery percentage smoothing
- ✔ Battery percentage curve calibration
- ✔ SleepService framework
- ✔ Deep sleep validation
- ✔ BOOT wake source
- ✔ Display dimming
- ✔ Display sleep
- ⬜ Automatic sleep
- ✔ Power policies

**Status:** In Progress

---

## ✅ Architecture Milestone 8 – MessageBus and Event Architecture

- ✔ MessageBus framework
- ✔ User-activity event flow
- ✔ Input-event publication from InputManager
- ✔ NavigationManager input subscription
- ✔ Navigation-change event flow
- ✔ ApplicationFrame synchronization through SentinelOS
- ✔ Core `InputEvent` contract
- ✔ Duplicate-subscription prevention
- ✔ Invalid-subscription validation
- ✔ Subscription-failure diagnostics
- ✔ Synchronous nested-dispatch documentation
- ✔ Hardware regression validation

**Status:** Complete

---

## ⬜ v0.6.0 – Wi-Fi Scanner

- SSID discovery
- RSSI
- Channel
- Security
- Vendor lookup
- Signal quality

---

## ⬜ v0.7.0 – Bluetooth Scanner

- BLE scanning
- Classic Bluetooth
- Beacon discovery
- Manufacturer decoding

---

## ⬜ v0.8.0 – Environmental Monitoring

- Temperature
- Humidity
- Pressure
- Air Quality
- Environmental dashboard

---

## ⬜ v0.9.0 – Network Analysis

- Packet capture
- Network discovery
- LLDP/CDP
- Ping
- DNS
- DHCP
- Gateway testing

---

## ⬜ v1.0.0 – Field Engineering Toolkit

- IP Camera discovery
- ONVIF tools
- PoE diagnostics
- Digital Concierge validation
- Cable testing
- Deployment utilities
- First production release