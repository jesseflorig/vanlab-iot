# Feature Specification: Remove Redundant feather_rgb_light Build Environment

**Feature Branch**: `005-remove-rgb-light-env`  
**Created**: 2026-04-18  
**Status**: Draft  
**Input**: User description: "Both the RP2040 shim and RP2040 feather boards have an onboard neopixel by default. I want to remove the redundant feather_rgb_light environment"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Configure NeoPixel Mode via Device YAML (Priority: P1)

A developer setting up a Feather as a Home Assistant RGB light no longer needs to choose a separate build environment for that capability. They use the standard Feather environment and declare the NeoPixel operating mode in the device YAML config file.

**Why this priority**: The core goal of the feature. The `feather_rgb_light` environment exists solely to set a compile-time flag — moving that decision to YAML eliminates the separate environment and aligns with the project's YAML-configurable design principle.

**Independent Test**: A developer builds and flashes the standard Feather environment with a YAML config that specifies NeoPixel light mode, and the resulting firmware registers as an HA RGB light — identical behavior to the old `feather_rgb_light` environment.

**Acceptance Scenarios**:

1. **Given** a device YAML specifying NeoPixel light mode, **When** the developer flashes using the standard Feather environment, **Then** the firmware behaves as an HA-controllable RGB light.
2. **Given** a device YAML specifying NeoPixel status mode (the default), **When** the developer flashes using the standard Feather environment, **Then** the firmware behaves as a status indicator — unchanged from today.
3. **Given** no NeoPixel mode specified in YAML, **When** the firmware boots, **Then** it defaults to status indicator mode without error.

---

### User Story 2 - Migrate Existing feather_rgb_light Device (Priority: P2)

A developer with an existing device that was flashed using the `feather_rgb_light` environment can migrate to the new approach by updating their device YAML config and reflashing with the standard Feather environment — preserving all HA RGB light functionality.

**Why this priority**: Existing users need a clear migration path. Without it, removing the environment breaks their workflow.

**Independent Test**: Take an existing `feather_rgb_light` device config, follow the migration steps, reflash with the standard environment, and confirm the device registers in Home Assistant identically to before.

**Acceptance Scenarios**:

1. **Given** an existing device config previously targeting `feather_rgb_light`, **When** the developer adds the NeoPixel light mode YAML field and reflashes using the standard Feather environment, **Then** the device registers in Home Assistant as an RGB light with the same device ID and topics.
2. **Given** the migration is complete, **When** the HA dashboard sends color/brightness commands, **Then** the NeoPixel responds correctly.

---

### User Story 3 - NeoPixel Light Mode Available on RP2040-Shim (Priority: P3)

Since both boards have an onboard NeoPixel, a developer using the RP2040-Shim can also configure NeoPixel light mode via YAML — a capability that was previously locked to the Feather-specific environment.

**Why this priority**: A natural consequence of the architecture change. Once mode selection moves to YAML, both boards gain the capability without any additional environment proliferation.

**Independent Test**: Build and flash the RP2040-Shim environment with a device YAML specifying NeoPixel light mode and confirm HA RGB light registration.

**Acceptance Scenarios**:

1. **Given** an RP2040-Shim device YAML specifying NeoPixel light mode, **When** the developer flashes using the `rp2040_shim` environment, **Then** the firmware registers as an HA RGB light.

---

### Edge Cases

- What happens if a device YAML specifies NeoPixel light mode but the board has no onboard NeoPixel at the expected pin?
- How does the firmware behave if the YAML specifies an unrecognized NeoPixel mode value?
- Are there any MQTT topic collisions if both NeoPixel status module and NeoPixel light module are accidentally active simultaneously?
- What should happen to the `devices/feather-rgb-light.yml` example config file — update in place, rename, or add a migration note?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The `feather_rgb_light` build environment MUST be removed from the build system configuration.
- **FR-002**: The NeoPixel operating mode (status indicator vs HA RGB light) MUST be selectable via device YAML configuration, not by choosing a different build environment.
- **FR-003**: Both the RP2040-Shim and RP2040 Feather environments MUST support both NeoPixel operating modes without requiring separate environments.
- **FR-004**: The default NeoPixel mode when unspecified in YAML MUST be status indicator (preserving current behavior for devices that don't declare a mode).
- **FR-005**: Devices migrated from `feather_rgb_light` MUST retain identical Home Assistant device IDs, MQTT topics, and discovery payloads.
- **FR-006**: An invalid or unrecognized NeoPixel mode value in YAML MUST cause a config validation error at startup, not silent misbehavior.
- **FR-007**: All documentation and example configs referencing `feather_rgb_light` MUST be updated or removed.
- **FR-008**: The native unit test suite MUST continue to pass after the change.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Zero references to `feather_rgb_light` remain in build configuration after the change.
- **SC-002**: A developer can achieve NeoPixel light mode on either board with only a YAML config change and a standard environment flash — no environment selection required.
- **SC-003**: All existing native tests pass after the change with no regressions.
- **SC-004**: An existing `feather_rgb_light` device can be migrated to the new approach in a single reflash cycle following documented steps.
- **SC-005**: Home Assistant sees no change in device ID or topic structure for a migrated device.

## Assumptions

- Both the Silicognition RP2040-Shim and Adafruit RP2040 Feather expose an onboard NeoPixel on the same GPIO pin that the firmware already expects — no pin remapping is needed.
- The `feather_rgb_light` environment's only meaningful difference from `feather_rp2040` is the `NEOPIXEL_LIGHT_MODE` compile flag and the device config file reference; all other settings are identical.
- The YAML config schema will be versioned following the constitution's requirement for breaking schema changes.
- The `devices/feather-rgb-light.yml` example file will be updated (not deleted) to serve as the migration reference.
- Backward compatibility at the environment name level is intentionally not preserved — the environment name `feather_rgb_light` is removed; this is the point of the feature.
