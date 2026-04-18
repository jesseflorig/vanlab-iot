# Implementation Plan: Remove Redundant feather_rgb_light Build Environment

**Branch**: `005-remove-rgb-light-env` | **Date**: 2026-04-18 | **Spec**: [spec.md](spec.md)  
**Input**: Feature specification from `specs/005-remove-rgb-light-env/spec.md`

## Summary

Remove the `feather_rgb_light` PlatformIO environment and replace the three `#ifdef NEOPIXEL_LIGHT_MODE` blocks in `main.cpp` with a runtime check on `DEVICE_CONFIG.standalone_modules[0].type`. The YAML `type` field (`"neopixel_light"` vs `"neopixel_status"`) already encodes the mode decision — `gen_config.py` already captures it — so no YAML schema changes are needed. The only code changes are in `main.cpp` and a null-guard addition to `LightBundle`'s constructor. Documentation is updated to remove all `feather_rgb_light` references.

## Technical Context

**Language/Version**: C++14 (Arduino framework)  
**Primary Dependencies**: Existing — PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (no new deps)  
**Storage**: N/A — `generated_config.h` regenerated at build time; no new persistent storage  
**Testing**: `pio test --environment native` — all existing suites must continue to pass  
**Target Platform**: RP2040 (both Adafruit Feather and Silicognition Shim)  
**Project Type**: Embedded firmware  
**Performance Goals**: No regression in RAM or flash usage; marginal static RAM increase (both module types compiled in) is acceptable on 264 KB SRAM  
**Constraints**: No heap allocation; C++14 only; no new build environments  
**Scale/Scope**: 3 `#ifdef` sites in `main.cpp`; 1 constructor null-guard; 1 environment removed; 4 documentation surfaces

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Offline-First Resilience | ✅ Not affected | No network path changes |
| II. Secure Transport | ✅ Not affected | No MQTT or TLS changes |
| III. HA Discovery | ✅ Preserved | NeoPixelLightModule's discovery payload is unchanged; same topics, same device ID |
| IV. YAML-Configurable Devices | ✅ **This feature advances this principle** | Moves mode selection from a compile flag to the YAML `type` field — strictly more configurable |
| V. Robust Testing | ✅ Maintained | All 10 native test suites are unaffected (modules are unchanged); `main.cpp` changes are not testable natively but are structurally simple |

**Gate result**: PASS — no violations. This feature is a direct expression of Principle IV.

## Project Structure

### Documentation (this feature)

```text
specs/005-remove-rgb-light-env/
├── plan.md                              ← this file
├── research.md                          ← Phase 0 complete
├── data-model.md                        ← Phase 1 complete
├── quickstart.md                        ← Phase 1 complete
├── contracts/
│   └── neopixel-module-type.md          ← Phase 1 complete
└── tasks.md                             ← Phase 2 output (/speckit.tasks)
```

### Source Code Changes

```text
platformio.ini                           ← remove [env:feather_rgb_light] section
src/main.cpp                             ← replace 3 #ifdef NEOPIXEL_LIGHT_MODE blocks
                                            with runtime isNeoPixelLightMode() check
src/bundles/light/LightBundle.cpp        ← add null-guard for empty BundleConfig
                                            (defensive: cfg.type == nullptr → no-op init)
README.md                                ← remove feather_rgb_light from env table
CLAUDE.md                                ← remove feather_rgb_light from Commands
specs/004-test-flash-feather/quickstart.md ← remove feather_rgb_light from env table
```

No new source files. `devices/feather-rgb-light.yml` is already correct — no changes needed.

## Key Design Decision

**The `isNeoPixelLightMode()` helper** (added to `main.cpp`):

```cpp
static bool isNeoPixelLightMode() {
    return DEVICE_CONFIG.standalone_module_count > 0
        && DEVICE_CONFIG.standalone_modules[0].type != nullptr
        && strcmp(DEVICE_CONFIG.standalone_modules[0].type, "neopixel_light") == 0;
}
```

Both `NeoPixelLightModule` and (`LightBundle` + `NeoPixelStatusModule`) are always statically allocated. In `setup()` and `loop()`, the active path is selected via `isNeoPixelLightMode()`. `LightBundle` requires a null-guard to safely handle construction when `DEVICE_CONFIG.bundle_count = 0` (i.e., neopixel_light configs that have no bundles).

## Complexity Tracking

*No constitution violations — section not applicable.*
