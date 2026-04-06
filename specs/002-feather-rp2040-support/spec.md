# Feature Specification: Adafruit RP2040 Feather Board Support

**Feature Branch**: `002-feather-rp2040-support`
**Created**: 2026-04-06
**Status**: Draft
**Input**: User description: "Add support for the Adafruit RP2040 Feather board (also to be paired with the Silicognition PoE FeatherWing)"

## Clarifications

### Session 2026-04-06

- Q: Should NeoPixel support be in scope, and if so for which boards? → A: Both boards — add a NeoPixel-capable status indicator module in this feature that works on both the Adafruit Feather RP2040 and the Silicognition RP2040-Shim.

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Build Feather firmware from YAML config (Priority: P1)

A developer configures a new device targeting the Adafruit RP2040 Feather + Silicognition PoE FeatherWing by copying a provided example YAML, setting `board: adafruit_feather_rp2040`, specifying their GPIO assignments using the Feather's pin numbering, and running a single build command. The firmware compiles and flashes without any code changes.

**Why this priority**: This is the entire point of the feature — a developer should be able to target the new board purely through configuration, with zero framework source edits.

**Independent Test**: Can be fully tested by building `devices/feather-example.yml` with `pio run -e feather_rp2040` and confirming the build succeeds and all native tests still pass.

**Acceptance Scenarios**:

1. **Given** a valid device YAML with `board: adafruit_feather_rp2040`, **When** the developer runs the build command, **Then** firmware compiles successfully with no errors.
2. **Given** a device YAML specifying a GPIO pin outside the Adafruit Feather RP2040's valid pin range, **When** the device config is loaded at boot, **Then** the firmware reports a config validation error.
3. **Given** the provided Feather example YAML, **When** the developer copies it and runs the build, **Then** a working firmware binary is produced without any code edits.

---

### User Story 2 — Ethernet connectivity on Feather + PoE FeatherWing (Priority: P2)

A developer flashes the Feather firmware and connects the board to an Ethernet cable via the PoE FeatherWing. The device boots, acquires a network address, and connects to the MQTT broker — identical runtime behavior to the Silicognition RP2040-Shim setup.

**Why this priority**: Without working Ethernet the device cannot publish MQTT messages or integrate with Home Assistant. P1 confirms the build; P2 confirms runtime.

**Independent Test**: Testable by flashing the Feather device and observing serial output confirming Ethernet initialization and MQTT connection.

**Acceptance Scenarios**:

1. **Given** a flashed Feather device with runtime credentials, **When** the device boots with an Ethernet cable connected, **Then** it initializes the W5500 chip on the PoE FeatherWing's SPI pins and acquires a network address.
2. **Given** Ethernet connectivity, **When** the device attempts to connect to the configured MQTT broker, **Then** it establishes a TLS session and publishes an availability message within 30 seconds of boot.
3. **Given** no Ethernet cable, **When** the device boots, **Then** it enters the MQTT reconnect backoff loop rather than hanging, and resumes normally when connectivity is restored.

---

### User Story 3 — NeoPixel status indicator on both boards (Priority: P3)

A developer assigns the onboard NeoPixel pin as a `status_led` module in their device YAML for either board (Feather RP2040 or RP2040-Shim). The NeoPixel displays a visible color-based status pattern, and the example YAMLs for both boards pre-configure the correct NeoPixel pin so no lookup is required.

**Why this priority**: Both boards have onboard NeoPixels but neither currently uses them. Adding NeoPixel support as a status indicator makes both boards' hardware fully utilized and gives developers a richer visual indicator than a plain LED.

**Independent Test**: Testable by observing the onboard NeoPixel showing the expected color pattern on each board after flash.

**Acceptance Scenarios**:

1. **Given** a device YAML with the board's NeoPixel pin assigned to a `status_led` module, **When** the firmware runs, **Then** the NeoPixel displays the framework's standard status color pattern.
2. **Given** either board's example YAML used unmodified, **When** the firmware boots, **Then** the NeoPixel status indicator activates without any user edits to the pin assignment.
3. **Given** a device YAML using the NeoPixel module on the Feather, **When** built and flashed, **Then** the NeoPixel on the Feather behaves identically to the NeoPixel on the RP2040-Shim.

---

### Edge Cases

- What happens when a Feather device YAML is accidentally built against the `rp2040_shim` environment (pin set mismatch between boards)?
- How does the PoE FeatherWing SPI pin assignment interact with Feather GPIO pins that share the SPI bus?
- What is the correct behavior if the PoE FeatherWing is physically absent but Ethernet is configured in the YAML?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The build system MUST support a `feather_rp2040` build environment targeting the Adafruit RP2040 Feather board.
- **FR-002**: Device config YAML MUST accept `board: adafruit_feather_rp2040` as a valid board identifier.
- **FR-003**: The framework MUST enforce the Adafruit Feather RP2040's valid GPIO pin set when validating a device config, rejecting pins that do not exist on the Feather headers.
- **FR-004**: The W5500 Ethernet chip on the Silicognition PoE FeatherWing MUST be initialized using the SPI CS, INT, and RST pin assignments defined by the FeatherWing's Feather-standard header layout.
- **FR-005**: All existing capabilities — MQTTS connectivity, bundle/module composition, HA Discovery, offline state buffering, YAML config codegen — MUST function identically on the Feather board as they do on the Silicognition RP2040-Shim.
- **FR-006**: A reference device YAML (`devices/feather-example.yml`) MUST be provided demonstrating a working Feather + PoE FeatherWing configuration with correct GPIO assignments and at least a status LED module.
- **FR-007**: The `feather_rp2040` build environment MUST share the same library dependencies (PubSubClient, ESP_SSLClient, ArduinoJson) as the `rp2040_shim` environment.
- **FR-008**: Existing `rp2040_shim` builds and all 62 native unit tests MUST continue to pass unchanged after this feature is introduced.
- **FR-009**: The framework MUST include a NeoPixel-capable status indicator module that works on both the Adafruit Feather RP2040 and the Silicognition RP2040-Shim via each board's respective onboard NeoPixel pin.
- **FR-010**: The NeoPixel module MUST be configurable via device YAML (pin assignment) and MUST display a distinct color pattern to convey device status, requiring no external hardware beyond the onboard NeoPixel.

### Key Entities

- **Board Target**: A named build environment + board identifier pairing (`feather_rp2040` / `adafruit_feather_rp2040`) that determines GPIO constraints and hardware initialization behavior.
- **PoE FeatherWing Pin Map**: The fixed SPI pin assignments (CS, INT, RST, SCK, MOSI, MISO) dictated by the Feather header standard for the Silicognition PoE FeatherWing — not user-configurable in YAML.
- **Feather GPIO Pin Set**: The set of GPIO pin numbers physically accessible on the Adafruit RP2040 Feather headers, distinct from the Silicognition RP2040-Shim's pin set.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: `pio run -e feather_rp2040` completes with zero errors on the reference example YAML.
- **SC-002**: `pio run -e rp2040_shim` and `pio test -e native` continue to produce zero regressions (62/62 tests pass) after the Feather environment is added.
- **SC-003**: A device YAML with an invalid Feather GPIO pin produces a detectable boot-time error rather than silently operating on the wrong pin.
- **SC-004**: A developer can target the Feather board by copying `devices/feather-example.yml`, editing only MQTT credentials, and running one build command — no framework source edits required.
- **SC-005**: The NeoPixel status indicator produces a visible color pattern on both the Feather and the RP2040-Shim onboard NeoPixels, verified by observation after flash on each board.

## Assumptions

- The Adafruit Feather RP2040 uses the earlephilhower Arduino core, the same as the Silicognition RP2040-Shim, so `RP2040GPIODriver` applies to both boards without modification.
- The Silicognition PoE FeatherWing uses fixed SPI pins determined by the Feather header standard; these are not user-configurable in device YAML.
- Both the Adafruit Feather RP2040 and the Silicognition RP2040-Shim have onboard NeoPixels; a NeoPixel-capable status indicator module is in scope for this feature and covers both boards.
- The simple red LED on the Adafruit Feather RP2040 (GPIO 13) is not used as the primary status indicator in this feature; the onboard NeoPixel is preferred.
- LiPo battery and USB charging behaviors of the Adafruit Feather are out of scope.
- Only the Adafruit Feather RP2040 is in scope for new board support; other Feather-form-factor MCUs (Feather M0, Feather ESP32, etc.) are not addressed.
- The PlatformIO board identifier for the Adafruit Feather RP2040 is `adafruit_feather_rp2040` (standard earlephilhower platform registry).
