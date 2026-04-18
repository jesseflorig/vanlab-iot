# Feature Specification: Validate Local Testing and Flashing to RP2040 Feather via USB-C

**Feature Branch**: `004-test-flash-feather`  
**Created**: 2026-04-18  
**Status**: Draft  
**Input**: User description: "Validate local testing and flashing to an RP2040 Feather via USB-C"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Run Native Unit Tests Locally (Priority: P1)

A developer working on firmware logic wants to confirm that the unit test suite runs cleanly on their local machine — without hardware attached — before committing or flashing.

**Why this priority**: This is the fastest feedback loop. All logic tests must pass before anything touches hardware. If this doesn't work, the entire development workflow is broken.

**Independent Test**: Developer runs the native test suite from a terminal and sees all tests pass with a clear pass/fail summary. No hardware required.

**Acceptance Scenarios**:

1. **Given** the project is checked out and dependencies are installed, **When** the developer runs the native test suite, **Then** all existing tests pass and results are printed to the terminal with a clear pass/fail count.
2. **Given** a test file contains a failing assertion, **When** the developer runs the test suite, **Then** the specific failing test is identified by name with the failure reason.
3. **Given** a new test file is added to the tests directory, **When** the developer runs the test suite, **Then** the new test is automatically discovered and executed without any manual registration.

---

### User Story 2 - Flash Firmware to Feather via USB-C (Priority: P2)

A developer has written or modified firmware and wants to flash it to an Adafruit RP2040 Feather connected via USB-C to confirm behavior on actual hardware.

**Why this priority**: Hardware validation is the ultimate correctness check. Native tests validate logic; flashing confirms the firmware runs on the real target.

**Independent Test**: Developer connects the Feather, runs a flash command for the correct environment, and the board reboots running the updated firmware.

**Acceptance Scenarios**:

1. **Given** the Feather is connected via USB-C and recognized by the OS, **When** the developer triggers a flash for the `feather_rp2040` or `feather_rgb_light` environment, **Then** the firmware compiles, uploads successfully, and the board reboots.
2. **Given** the board is not connected, **When** the developer attempts to flash, **Then** a clear error indicates no upload target was found.
3. **Given** the `gen_config.py` pre-build script runs during the flash, **When** the target device YAML config is valid, **Then** a `generated_config.h` is produced and included in the build without manual intervention.

---

### User Story 3 - Confirm Serial Monitor Output After Flash (Priority: P3)

A developer wants to observe the board's serial output after flashing to verify startup behavior, MQTT connection attempts, and module initialization.

**Why this priority**: Serial output is the primary diagnostic tool for post-flash debugging. Without it, confirming correct behavior on hardware requires guesswork.

**Independent Test**: Developer opens a serial monitor after flashing and sees structured output from the firmware's startup sequence.

**Acceptance Scenarios**:

1. **Given** the Feather is flashed and running, **When** the developer opens a serial monitor at the configured baud rate, **Then** firmware startup messages are visible including any MQTT or module initialization output.
2. **Given** the serial monitor is open and the board is reset, **When** the board restarts, **Then** the startup sequence output appears again without needing to reopen the monitor.

---

### Edge Cases

- What happens when the Feather is in BOOTSEL mode (mass storage) rather than normal run mode when a flash is triggered?
- How does the build behave if `devices/feather-example.yml` or `devices/feather-rgb-light.yml` is malformed or missing?
- What happens if a native test imports a hardware-dependent file that was not excluded by `build_src_filter`?
- How does the toolchain behave if the USB-C cable is data-only (charging only, no data lines)?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The native test suite MUST execute all tests in the `tests/` directory without requiring physical hardware connected.
- **FR-002**: The native build MUST exclude hardware-dependent source files (`main.cpp`, `MQTTClientWrapper.cpp`) so tests compile and run on the host machine.
- **FR-003**: The developer MUST be able to flash firmware to an Adafruit RP2040 Feather using a single command targeting either the `feather_rp2040` or `feather_rgb_light` environment.
- **FR-004**: The pre-build config generation script MUST run automatically before any firmware build — native tests and hardware flash — without manual invocation.
- **FR-005**: The serial monitor MUST be accessible at 115200 baud after a successful flash.
- **FR-006**: The build system MUST apply the correct board-level and feature-level compile flags (`BOARD_FEATHER_RP2040`, `NEOPIXEL_LIGHT_MODE`, etc.) for each environment without manual flag injection.
- **FR-007**: All test results MUST clearly identify passing and failing tests by name so failures can be acted on without reading raw output.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A developer can run the full native test suite and receive a pass/fail result in under 60 seconds from a clean checkout with dependencies installed.
- **SC-002**: All existing tests pass on the native platform with zero failures on a clean run.
- **SC-003**: A firmware flash to a connected Feather completes successfully end-to-end (compile + upload + reboot) in under 3 minutes.
- **SC-004**: Serial output is visible within 10 seconds of the board rebooting after a flash.
- **SC-005**: A developer unfamiliar with the project can run tests and flash the board by following project documentation without external assistance.

## Assumptions

- The developer's machine has PlatformIO CLI installed and accessible from the terminal.
- The Feather board is an Adafruit RP2040 Feather and is connected via a USB-C cable capable of data transfer (not charge-only).
- The OS recognizes the Feather as a USB serial device; driver installation is not in scope.
- Device YAML config files (`devices/feather-example.yml`, `devices/feather-rgb-light.yml`) are present and valid in the repository.
- The `native` PlatformIO environment is the designated target for running tests locally without hardware.
- Project documentation (README or CLAUDE.md) is the expected place to surface the commands — this spec does not mandate a GUI or wrapper script.
