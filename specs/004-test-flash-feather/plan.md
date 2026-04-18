# Implementation Plan: Validate Local Testing and Flashing to RP2040 Feather via USB-C

**Branch**: `004-test-flash-feather` | **Date**: 2026-04-18 | **Spec**: [spec.md](spec.md)  
**Input**: Feature specification from `specs/004-test-flash-feather/spec.md`

## Summary

Confirm that the developer workflow for running native unit tests and flashing firmware to an Adafruit RP2040 Feather via USB-C is functional and documented. The deliverables are: verified test execution, corrected README Getting Started commands (a documented bug where the environment name `rp2040` does not exist), an updated CLAUDE.md Commands section, and a quickstart guide.

## Technical Context

**Language/Version**: C++14 (Arduino framework); Python 3.x (build scripts)  
**Primary Dependencies**: PlatformIO CLI, PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel  
**Storage**: N/A — no new storage; `generated_config.h` and LittleFS config unchanged  
**Testing**: PlatformIO native environment (Unity under the hood); `pio test --environment native`  
**Target Platform**: Adafruit RP2040 Feather (USB-C flash) + host machine (native tests)  
**Project Type**: Embedded firmware  
**Performance Goals**: Native tests complete in <60 sec; flash completes in <3 min  
**Constraints**: RP2040 — 264 KB SRAM, 2 MB flash; no Wi-Fi  
**Scale/Scope**: Single developer workflow; no new code modules

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Offline-First Resilience | ✅ Not applicable | No new network code |
| II. Secure Transport | ✅ Not applicable | No new MQTT code |
| III. HA Discovery | ✅ Not applicable | No new modules |
| IV. YAML-Configurable Devices | ✅ Satisfied | Existing device YAMLs are unchanged; flash workflow reads them via `gen_config.py` |
| V. Robust Testing | ✅ Central to this feature | This feature validates that unit tests run on host; no new modules added without tests |

**Gate result**: PASS. No violations. Proceed to research.

## Project Structure

### Documentation (this feature)

```text
specs/004-test-flash-feather/
├── plan.md          ← this file
├── research.md      ← Phase 0 complete
├── quickstart.md    ← Phase 1 complete
└── tasks.md         ← Phase 2 output (/speckit.tasks — not yet created)
```

*No data-model.md or contracts/: this feature adds no new data entities or external interfaces.*

### Source Code Changes

```text
README.md                         ← fix Getting Started environment name + add test command
CLAUDE.md                         ← populate Commands section with pio commands
```

No `src/` or `tests/` changes. This is a documentation-only feature.

## Complexity Tracking

*No constitution violations — section not applicable.*
