# Implementation Plan: Feather NeoPixel RGB Light

**Branch**: `003-feather-neopixel-light` | **Date**: 2026-04-07 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-feather-neopixel-light/spec.md`

## Summary

Add a Home Assistant-controllable RGB light module that drives the RP2040 Feather's onboard NeoPixel (GPIO 16) via MQTT. The module subscribes to an HA light command topic, applies on/off/color/brightness commands to the NeoPixel through the existing `INeoPixelDriver` interface, publishes state back to MQTT, and participates in the existing reconnect/resubscribe lifecycle. A small extension to `IModule` (adding a `resubscribe()` virtual method) and a corresponding update to `BundleRegistry::resubscribeAll()` enable standalone modules to self-subscribe to MQTT topics — closing an existing framework gap without architectural disruption.

## Technical Context

**Language/Version**: C++14, Arduino framework (earlephilhower arduino-pico core)
**Primary Dependencies**: PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (all existing — no new deps)
**Storage**: None (last-commanded color retained in-memory only; no flash persistence)
**Testing**: Unity (PlatformIO native test runner) — `pio test -e native`
**Target Platform**: Adafruit RP2040 Feather + Silicognition PoE FeatherWing (`feather_rp2040` env)
**Project Type**: Embedded firmware module integrating with existing Orchestrator + BundleRegistry framework
**Performance Goals**: NeoPixel responds within 1 second of command receipt (MQTT delivery latency dominated)
**Constraints**: `handleCommand()` must be MQTT-agnostic (pure logic callable from any callback); no `delay()` in loop; no flash writes; `IModule::loop()` does not receive MQTTClientWrapper — module self-subscribes via `resubscribe()` instead
**Scale/Scope**: Single-pixel NeoPixel; single device config; 1:1 with HA `light` entity

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| Testability without hardware | PASS | `handleCommand()` is MQTT-agnostic; unit tests use `MockNeoPixelDriver` only |
| No framework coupling in modules | PASS | NeoPixelLightModule depends on `INeoPixelDriver` (interface), not concrete driver |
| Config-driven behavior | PASS | Module registered from `devices/feather-rgb-light.yml` via codegen |
| Backward compatibility | PASS | `IModule::resubscribe()` default no-op preserves all existing modules |
| Single responsibility | PASS | Light control in NeoPixelLightModule; discovery/state publish self-contained |

**Post-Phase-1 re-check**: All gates still pass. `resubscribe()` extension is additive only.

## Project Structure

### Documentation (this feature)

```text
specs/003-feather-neopixel-light/
├── plan.md              # This file
├── research.md          # Phase 0: architectural decisions
├── data-model.md        # Phase 1: entity definitions
├── quickstart.md        # Phase 1: developer walkthrough
├── contracts/           # Phase 1: MQTT topic contract
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (repository root)

```text
include/
├── IModule.h                          # EXTEND: add virtual resubscribe(MQTTClientWrapper&) {}
├── INeoPixelDriver.h                  # Existing — no change
└── vanlab-iot.h                       # ADD: NeoPixelLightModule include

src/
├── bundles/
│   ├── base/
│   │   ├── BundleRegistry.h           # No change (resubscribeAll signature unchanged)
│   │   └── BundleRegistry.cpp         # EXTEND: call module.resubscribe() in resubscribeAll()
│   └── modules/
│       └── neopixel/
│           ├── NeoPixelStatusModule.h  # Existing — no change
│           ├── NeoPixelStatusModule.cpp# Existing — no change
│           ├── NeoPixelLightModule.h   # NEW
│           └── NeoPixelLightModule.cpp # NEW
└── main.cpp                           # ADD: MQTT callback; conditional module registration

devices/
└── feather-rgb-light.yml              # NEW: reference device config

tests/
└── test_neopixel_light/
    └── test_neopixel_light.cpp        # NEW: unit tests (MockNeoPixelDriver)
```

**Structure Decision**: Extends the existing single-project embedded firmware layout. No new directories beyond the established `src/bundles/modules/neopixel/` pattern from feature 002. New test suite follows the `tests/test_<module>/test_<module>.cpp` convention.

## Hardware Reference

| Item | Value |
|------|-------|
| NeoPixel GPIO | 16 (onboard, Feather) |
| PlatformIO env | `feather_rp2040` |
| NeoPixel driver | `RP2040NeoPixelDriver` (from feature 002) |
| HA entity type | `light` |
| Command topic | `{topic_root}/light/rgb/set` |
| State topic | `{topic_root}/light/rgb/state` |
| Discovery topic | `homeassistant/light/{device_id}/config` |
| Command format | `{"state":"ON","color":{"r":255,"g":0,"b":0},"brightness":255}` |
| State format | `{"state":"ON","color":{"r":255,"g":0,"b":0},"brightness":255}` |
