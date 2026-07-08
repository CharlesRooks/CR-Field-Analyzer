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