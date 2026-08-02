# SentinelOS Architecture Decision Records (ADR)

---

## ADR-001

### Title

Gesture-Based Navigation

### Status

Accepted

### Date

2026-07-06

### Decision

SentinelOS will use horizontal swipe gestures as the primary navigation mechanism between pages.

The Navigation Bar serves only as an indicator of the current location within the application.

### Rationale

The 1.91-inch AMOLED display does not provide sufficient space for reliable touch buttons while maintaining good readability.

Gesture navigation:

- Improves one-handed operation.
- Reduces touch precision requirements.
- Better suits field environments.
- Leaves more screen space available for useful information.

### Consequences

NavigationManager will become gesture-driven.

NavigationBar will display application state rather than initiate navigation.

Future page transitions may include horizontal slide animations.

---

## ADR-002

### Title

Pages Own Content Only

### Status

Accepted

### Date

2026-07-06

### Decision

Application pages own only the content displayed within the Content Area.

Shared interface elements such as the TitleBar and NavigationBar are provided by the Page framework.

### Rationale

Separating shared UI from page content:

- Improves consistency.
- Eliminates duplicated code.
- Simplifies page development.
- Allows global UI changes without modifying individual pages.

### Consequences

Future pages only implement:

- CreateContent()
- Update()

All common interface elements are managed by the framework.

## ADR-003

### Title
InputManager Abstracts Touch Hardware

### Status
Accepted

### Decision
All touch input is processed by InputManager before being exposed to the rest of SentinelOS.

### Rationale
The touch controller does not provide stable continuous press events. InputManager normalizes raw touch data into high-level events such as SwipeLeft, SwipeRight, and Tap, isolating hardware-specific behavior from the rest of the application.

### Consequences
Future hardware changes will require updates only to InputManager (or the underlying touch service), while NavigationManager and application pages remain unchanged.

---

## ADR-004

### Title

Synchronous MessageBus for Event Distribution

### Status

Accepted

### Date

2026-08-02

### Decision

SentinelOS will use a lightweight, fixed-capacity MessageBus for event distribution between Core, Managers, and top-level application coordination.

The MessageBus:

- Uses static function-pointer handlers.
- Supports a maximum of 16 subscriptions.
- Dispatches messages synchronously.
- Allows handlers to publish additional messages during dispatch.
- Does not queue, retain, or replay messages.
- Treats an identical existing subscription as successful without registering it twice.
- Rejects `MessageType::None`, null handlers, and registrations beyond capacity.

Shared event contracts, including `InputEvent`, belong in Core rather than inside an event-producing Manager.

### Rationale

The embedded platform requires predictable memory use, minimal runtime overhead, and loose coupling between input, navigation, power policy, and future application modules.

A synchronous fixed-capacity bus:

- Avoids dynamic allocation.
- Provides deterministic dispatch.
- Keeps event producers independent from consumers.
- Supports nested flows such as `InputEvent` producing `NavigationChanged`.
- Provides a scalable foundation for future Wi-Fi, Bluetooth, notification, and system events.

Moving `InputEvent` into Core prevents the messaging layer from depending on `InputManager` and establishes the correct dependency direction.

### Consequences

- Handlers execute before `Publish()` returns.
- Handlers must be short and non-blocking.
- Handlers must not change subscriptions during dispatch.
- Publishers must not assume messages are queued for later processing.
- Callers must check `Subscribe()` and log failures.
- Active event flows are currently `UserActivity`, `InputEvent`, and `NavigationChanged`.
- Power, display, sleep, application, Wi-Fi, and notification message types remain reserved until their publishers and subscribers are implemented.
