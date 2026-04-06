# Implementation Plan: Core Framework

**Branch**: `001-core-framework` | **Date**: 2026-04-05 | **Spec**: N/A (bootstrap feature)
**Input**: "Build initial project infra and capabilities"

## Summary

Establish the complete vanlab-iot framework skeleton: PlatformIO build system, HAL
abstraction layer, YAML→C++ config codegen pipeline, MQTTS client wrapper with offline
reconnect FSM, HA Discovery payload builder, bundle + module interfaces, and orchestration
core. The runtime hierarchy is **device → bundle → module**, with standalone modules
supported alongside bundles. All subsequent device capabilities will build on this
foundation.

## Technical Context

**Language/Version**: C++14, Arduino framework
**Primary Dependencies**: PubSubClient (MQTT), ESP_SSLClient (TLS/W5500), ArduinoJson
(runtime config), ArduinoFake (test mocks), maxgerhardt/platform-raspberrypi (platform)
**Storage**: LittleFS 1MB partition — runtime JSON config + TLS certificates
**Testing**: PlatformIO `[env:native]` + Unity + ArduinoFake + HAL injection pattern
**Target Platform**: RP2040 (Silicognition RP2040-Shim + FeatherWing PoE), board ID:
`silicognition_rp2040_shim`, earlephilhower arduino-pico core
**Project Type**: Embedded framework/library
**Performance Goals**: Main loop non-blocking; reconnect FSM completes within one loop
tick; HA Discovery payload < 1KB per entity; event bus dispatch O(n) subscribers, n ≤ 8
**Constraints**: 264 KB SRAM; 2 MB flash (1MB firmware / 1MB LittleFS); offline-first;
TLS mandatory; no `delay()` in `loop()`; bundle behaviors hardcoded in C++ (no rule engine)
**Scale/Scope**: Single-device framework; up to 8 bundles per device; up to 16 modules
per device (across all bundles + standalone)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Offline-First Resilience | ✅ PASS | `StateBuffer` + reconnect FSM; bundle event bus enables offline inter-module coordination without MQTT |
| II. Secure Transport | ✅ PASS | MQTTS only via ESP_SSLClient; no plaintext MQTT path exists |
| III. HA MQTT Discovery | ✅ PASS | Bundles publish per-entity discovery payloads sharing device object; batched on every reconnect |
| IV. YAML-Configurable Devices | ✅ PASS | Build-time YAML→C++ codegen; bundles + standalone modules declared in YAML; GPIO validation at compile time |
| V. Robust Testing | ✅ PASS | `IGPIODriver` + `MockGPIODriver`; `MockEventBus` for bundle testing; native PlatformIO env; ArduinoFake |

No violations. Complexity Tracking not required.

## Clarifications

### Session 2026-04-05

- Q: Should bundles be enforced at build time or be full runtime entities? → A: Full runtime entities — device → bundle → module hierarchy at runtime
- Q: How do modules within a bundle coordinate? → A: Bundle-mediated typed event bus; modules publish/subscribe through the bundle; no direct references between siblings
- Q: How should bundles appear in Home Assistant? → A: Per-entity discovery payloads sharing a single `device` object; all entities group under one HA device card; framework batches all discovery publishes on connect
- Q: Must every module belong to a bundle? → A: No — standalone modules supported; orchestrator manages bundles and standalone modules in the same loop
- Q: How are bundle behaviors specified? → A: Fully hardcoded in C++ per bundle type; no YAML rule engine; behavior changes require reflash; keeps runtime leanest

## Project Structure

### Documentation (this feature)

```text
specs/001-core-framework/
├── plan.md              # This file
├── research.md          # Phase 0 — library and architecture decisions
├── data-model.md        # Phase 1 — entity definitions and state machines
├── quickstart.md        # Phase 1 — developer onboarding guide
├── contracts/
│   ├── bundle-interface.md       # IBundle contract (added post-clarification)
│   ├── module-interface.md       # IModule contract
│   ├── hal-interface.md          # IGPIODriver contract
│   ├── device-config-schema.md   # YAML config schema v1
│   ├── mqtt-topics.md            # Topic structure and payload formats
│   └── ha-discovery.md           # HA MQTT Discovery payload spec
└── tasks.md             # Created by /speckit-tasks (not yet)
```

### Source Code (repository root)

```text
include/
├── IBundle.h             # Bundle interface (pure virtual)
├── IModule.h             # Module interface (pure virtual)
├── IGPIODriver.h         # GPIO HAL interface (pure virtual)
└── vanlab-iot.h          # Framework public entry point

src/
├── core/
│   └── Orchestrator.h/.cpp   # Main loop, init; manages BundleRegistry + standalone modules
├── config/
│   ├── ConfigTypes.h         # DeviceConfig, BundleConfig, ModuleConfig, RuntimeConfig
│   ├── ConfigLoader.h/.cpp   # LittleFS JSON runtime config loader
│   └── generated_config.h    # Build-time output of scripts/gen_config.py (gitignored)
├── mqtt/
│   ├── MQTTClientWrapper.h/.cpp   # PubSubClient + ESP_SSLClient + reconnect FSM
│   ├── StateBuffer.h/.cpp         # Offline message ring buffer
│   └── HADiscoveryPayload.h/.cpp  # HA Discovery payload builder
├── hal/
│   └── RP2040GPIODriver.h    # Arduino-core IGPIODriver implementation
└── bundles/
    ├── base/
    │   ├── BundleRegistry.h/.cpp  # Bundle + standalone module orchestration
    │   └── BundleEventBus.h/.cpp  # Typed intra-bundle event bus
    ├── light/
    │   └── LightBundle.h/.cpp     # Dimmer + physical switch coordination
    └── modules/
        ├── dimmer/
        │   └── DimmerModule.h/.cpp
        ├── physical_switch/
        │   └── PhysicalSwitchModule.h/.cpp
        └── status_led/
            └── StatusLEDModule.h/.cpp

main.cpp                      # Arduino entry point; instantiates + drives Orchestrator

tests/
├── mocks/
│   ├── MockGPIODriver.h      # IGPIODriver test double
│   └── MockEventBus.h        # BundleEventBus test double
└── unit/
    ├── test_state_buffer.cpp
    ├── test_mqtt_fsm.cpp
    ├── test_ha_discovery.cpp
    ├── test_config_loader.cpp
    ├── test_event_bus.cpp
    ├── bundles/
    │   └── test_light_bundle.cpp
    └── modules/
        └── test_status_led_module.cpp

devices/
└── example.yml               # Template device config

scripts/
├── gen_config.py             # YAML → generated_config.h codegen
└── gen_certs.py              # PEM cert → BearSSL trust_anchors.h

data/                         # LittleFS filesystem root (uploadfs target)
├── config/
│   └── runtime.json.example
└── certs/
    └── .gitkeep

platformio.ini
```

**Structure Decision**: Single-project layout. Bundles live in `src/bundles/`; modules
are nested under `src/bundles/modules/` as shared primitives any bundle can compose.
Standalone modules are registered directly with the `BundleRegistry`. The `native`
PlatformIO env shares `src/` via `test_build_src = yes` for host-side testing.
