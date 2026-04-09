# Feature Specification: Feather NeoPixel RGB Light

**Feature Branch**: `003-feather-neopixel-light`
**Created**: 2026-04-06
**Status**: Draft
**Input**: User description: "Put together a baseline device configuration for an RP2040 Feather that connects to an MQTT server, publishes an HA Discovery message, and registers an RGB light switch that controls the onboard neopixel"

## Clarifications

### Session 2026-04-07

- Q: Should the last commanded color survive a power cycle so the NeoPixel restores state on boot without waiting for HA? → A: No persistence. NeoPixel is off on cold boot; HA restores last state within seconds of reconnect via the existing availability/reconnect pattern.
- Q: What implementation language and stack should this feature use? → A: C++14, Arduino framework (earlephilhower arduino-pico core), PlatformIO `feather_rp2040` env — same stack as features 001/002. No new language or runtime choice required; module must integrate with the existing Orchestrator, BundleRegistry, and INeoPixelDriver interfaces.

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Register as a controllable RGB light in Home Assistant (Priority: P1)

A developer flashes an RP2040 Feather with the `feather-rgb-light` device config. On boot, the device connects to the MQTT broker and publishes a Home Assistant Discovery message. The light entity appears automatically in Home Assistant — with no manual HA configuration — and is ready for user control.

**Why this priority**: Without HA Discovery, the device has no user-visible presence in Home Assistant. This is the minimum required for the feature to deliver value.

**Independent Test**: Flash the device and observe in Home Assistant that a new `light` entity appears automatically with the correct device name and supports RGB color control.

**Acceptance Scenarios**:

1. **Given** a flashed Feather device with valid MQTT credentials, **When** the device boots and connects, **Then** a `light` entity appears in Home Assistant automatically without manual entity registration.
2. **Given** the HA light entity is present, **When** the device reconnects after a reboot, **Then** the light entity remains registered and available (HA Discovery is republished on reconnect).
3. **Given** the `feather-rgb-light` device YAML, **When** a developer copies it, fills in MQTT credentials, and flashes, **Then** the device is fully registered in HA with no additional steps.

---

### User Story 2 — Control the onboard NeoPixel from Home Assistant (Priority: P2)

A Home Assistant user turns the Feather's onboard NeoPixel on or off and changes its color using the standard HA light card. The NeoPixel responds immediately to commands. The device publishes its current state back to MQTT so HA reflects the actual light status.

**Why this priority**: The controllable light is the core value of this feature. Without it, HA Discovery alone delivers no functional benefit.

**Independent Test**: From HA, send an ON command with a specific RGB color. Observe the onboard NeoPixel changes to that color within one second. Send OFF. Observe the NeoPixel turns off.

**Acceptance Scenarios**:

1. **Given** the HA light entity, **When** a user sends an ON command with a color (e.g., red), **Then** the NeoPixel changes to that color within one second and HA reflects the new state.
2. **Given** the NeoPixel is on, **When** a user sends an OFF command, **Then** the NeoPixel turns off and the HA entity state shows OFF.
3. **Given** the NeoPixel is off, **When** a user sends an ON command without a color, **Then** the NeoPixel restores the last color used (or a default if no prior color).
4. **Given** the NeoPixel is displaying a color, **When** a user sends a brightness command, **Then** the NeoPixel scales its brightness while preserving the current hue.

---

### User Story 3 — State survives disconnection and reconnection (Priority: P3)

When the Feather loses its MQTT connection and later reconnects, the NeoPixel retains its last commanded state during the outage. On reconnect, the device republishes its current state so Home Assistant is immediately in sync — no manual refresh required.

**Why this priority**: State consistency is expected behavior for any HA-integrated device. Users should not need to re-send commands after a network blip.

**Independent Test**: Command the NeoPixel to red. Disconnect the network for 30 seconds. Reconnect. Observe HA reflects the correct light state without any user action within five seconds.

**Acceptance Scenarios**:

1. **Given** the NeoPixel is displaying a color, **When** the device loses MQTT connectivity, **Then** the NeoPixel continues displaying that color.
2. **Given** the device has reconnected, **When** HA queries the light state, **Then** the state matches the last commanded state before disconnection.
3. **Given** a command is sent from HA while the device is offline, **When** the device reconnects, **Then** the buffered command is applied and the NeoPixel updates accordingly.

---

### Edge Cases

- On cold boot (first power-on or after power loss), the NeoPixel is off until HA sends the first command. No prior state is stored on-device; HA is the source of truth for light state.
- What happens if an invalid or out-of-range RGB value is received in a command?
- What is the device's behavior if HA sends a brightness-only command while the light is OFF?
- The onboard NeoPixel (GPIO 16) is also used by `NeoPixelStatusModule` from feature 002 — can both be active simultaneously in the same config?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The device MUST connect to the configured MQTT broker and publish an HA Discovery message for a `light` entity that supports RGB color and brightness control.
- **FR-002**: The `light` entity MUST appear automatically in Home Assistant without manual entity registration.
- **FR-003**: The device MUST respond to HA light commands (on, off, RGB color, brightness) by updating the onboard NeoPixel within one second of receiving the command.
- **FR-004**: The device MUST publish its current light state back to MQTT after each state change so Home Assistant reflects the actual NeoPixel state.
- **FR-005**: The device MUST republish its HA Discovery message and current state on every MQTT reconnection.
- **FR-006**: The NeoPixel MUST retain its last commanded state if the MQTT connection is lost; it MUST NOT turn off or change color due to a disconnection alone.
- **FR-007**: Commands received from HA while the device is offline MUST be buffered and applied when connectivity is restored.
- **FR-008**: A reference device YAML (`devices/feather-rgb-light.yml`) MUST be provided demonstrating a working Feather RGB light configuration targeting GPIO 16.
- **FR-009**: Existing native unit tests MUST continue to pass after this feature is introduced.

### Key Entities

- **RGB Light**: A user-controllable light with on/off state, RGB color (0–255 per channel), and brightness (0–255). Last commanded color is retained in memory; not persisted across power cycles.
- **NeoPixel Command**: An inbound control message from Home Assistant specifying one or more of: state (ON/OFF), RGB color, brightness.
- **Light State**: The device's current NeoPixel output — on/off, active color, active brightness — published to MQTT after each change.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After flashing, the HA `light` entity appears within 30 seconds of the device booting with network connectivity — no manual HA configuration required.
- **SC-002**: The NeoPixel responds to an HA command within one second of the command being received by the device.
- **SC-003**: A developer can produce a working HA-registered NeoPixel light by copying `devices/feather-rgb-light.yml`, filling in MQTT credentials, and running one build command — zero framework source edits required.
- **SC-004**: All existing native unit tests continue to pass with no regressions after this feature is added.
- **SC-005**: HA correctly reflects the current NeoPixel state within five seconds of the device reconnecting after a 60-second network outage.

## Assumptions

- The onboard NeoPixel (GPIO 16) is used exclusively for RGB light control in this configuration. `NeoPixelStatusModule` (feature 002) is NOT active simultaneously in the same device config; the two YAML configs are separate and not combined.
- Brightness control is achieved by scaling the RGB values uniformly (brightness 128 at full red → R=128, G=0, B=0), not via a separate PWM pin.
- Color persistence across power cycles is explicitly out of scope. The NeoPixel is off on cold boot; HA pushes the last known state within seconds of the device reconnecting via the existing availability topic and reconnect pattern. Flash wear and write-timing complexity are avoided.
- The HA light command format follows the standard Home Assistant JSON light command schema: `{"state": "ON", "color": {"r": 255, "g": 0, "b": 0}, "brightness": 255}`.
- The device uses the same MQTT broker, TLS configuration, and runtime secret mechanism established in feature 001.
- GPIO 16 is physically internal to the Feather board; no external wiring is required.
- This feature uses the same implementation stack as features 001/002: C++14, Arduino framework (earlephilhower arduino-pico core), PlatformIO `feather_rp2040` env. The new module must integrate with the existing Orchestrator, BundleRegistry, and `INeoPixelDriver` interfaces.
- This feature builds on the `feather_rp2040` PlatformIO environment and `INeoPixelDriver` / `RP2040NeoPixelDriver` introduced in feature 002.
