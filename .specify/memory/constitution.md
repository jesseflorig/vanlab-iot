<!--
SYNC IMPACT REPORT
==================
Version change: (unversioned template) → 1.0.0
Type: MAJOR — first ratified version; all sections new

Added sections:
  - Core Principles (5 principles: all new)
  - Hardware Platform Constraints (new)
  - Development Workflow (new)
  - Governance (new)

Modified principles: N/A (first ratification)

Templates reviewed:
  - .specify/templates/plan-template.md ✅ — Constitution Check is generic directive; compatible
  - .specify/templates/spec-template.md ✅ — No constitution-specific references; compatible
  - .specify/templates/tasks-template.md ✅ — No constitution-specific references; compatible

Deferred TODOs: none
-->

# vanlab-iot Constitution

## Core Principles

### I. Offline-First Resilience

Devices MUST operate autonomously without a network connection. Loss of MQTT broker
connectivity MUST NOT cause device failure or data loss. Devices MUST:

- Buffer state locally when the broker is unreachable.
- Reconnect automatically with exponential backoff.
- Never block the main control loop on network I/O.
- Resume publishing buffered state upon reconnection.

**Rationale**: PoE/ethernet links can drop; devices must remain useful and self-consistent
without the broker present.

### II. Secure Transport (NON-NEGOTIABLE)

All MQTT communication MUST use MQTTS (TLS). Plaintext MQTT is forbidden.

- TLS certificates MUST be provisioned per-device.
- Certificate validation MUST be enabled; `setInsecure()` bypasses are forbidden in production.
- Broker credentials MUST be stored in YAML config, never compiled into firmware.

**Rationale**: These are network-connected embedded devices; plaintext credentials on a
local network are an unacceptable attack surface.

### III. Home Assistant MQTT Discovery

Every device module MUST publish a valid Home Assistant MQTT Discovery payload on startup
and on reconnect.

- Topics MUST follow the `homeassistant/<component>/<device_id>/<object_id>/config` pattern.
- Payloads MUST include `device`, `unique_id`, `availability_topic`, and `state_topic`.
- Discovery payloads MUST be published with `retain: true`.

**Rationale**: HA Discovery is the integration contract. Devices that skip it require
manual HA configuration, which defeats the framework's automation goal.

### IV. YAML-Configurable Devices

Device behavior, GPIO pin assignments, MQTT topics, and module composition MUST be
defined in YAML configuration. No GPIO pin numbers, topic strings, or module parameters
may be hardcoded in firmware.

- Each device MUST have a corresponding YAML config file committed to version control.
- GPIO assignments MUST be validated against the target board's pinout at config load time.
- Config schema MUST be versioned; breaking schema changes require a schema version bump
  and a migration note.

**Rationale**: Configuration-driven design enables the same firmware to serve multiple
device roles and makes device setup auditable and reproducible.

### V. Robust Testing

All framework modules MUST have unit tests. Hardware-dependent code MUST be abstracted
behind mockable HAL interfaces to enable host-side testing without physical hardware.

- Unit tests MUST pass on host (desktop) before targeting hardware.
- GPIO and peripheral interactions MUST use mockable abstractions.
- Integration tests MUST cover the full MQTTS publish/subscribe cycle end-to-end.
- New modules MUST include tests before merging.

**Rationale**: Embedded bugs are difficult to debug in the field. A strong test suite
catches regressions before they reach hardware.

## Hardware Platform Constraints

The canonical target hardware is:

- **MCU**: Raspberry Pi RP2040 (dual-core Cortex-M0+, 264 KB SRAM, 2 MB flash)
- **Boards**: Silicognition RP2040-Shim and Silicognition FeatherWing PoE
- **Connectivity**: Ethernet via PoE (W5500 or equivalent); no Wi-Fi dependency
- **Toolchain**: Arduino framework with C++ (C++14 minimum)

All framework code MUST compile and run on this hardware combination. Features requiring
capabilities absent from this platform (Wi-Fi, BLE, PSRAM) are out of scope unless
explicitly contained in an optional module.

Modules MUST document their approximate RAM and flash footprint. Memory budget awareness
is a first-class constraint given the RP2040's 264 KB SRAM.

## Development Workflow

- Features MUST be developed on a branch and reviewed before merging to `main`.
- Every PR MUST include a Constitution Check verifying compliance with all five principles.
- Breaking changes to the YAML config schema MUST bump the schema version and include a
  migration note in the PR description.
- The `main` branch MUST always represent a state that compiles and passes all tests.
- Device configs MUST live in version control alongside firmware.

## Governance

This constitution supersedes all other practices and documentation in this repository.
Amendments MUST be proposed as a PR to `.specify/memory/constitution.md`, include a
rationale, and increment the version according to semantic versioning:

- **MAJOR**: Removal or redefinition of a principle; backward-incompatible governance change.
- **MINOR**: New principle or section added; materially expanded guidance.
- **PATCH**: Clarifications, wording, typo fixes, non-semantic refinements.

All PRs and code reviews MUST verify compliance with the Core Principles before approval.
Complexity that violates a principle MUST be justified in the PR description.

**Version**: 1.0.0 | **Ratified**: 2026-04-05 | **Last Amended**: 2026-04-05
