# Implementation Plan: Adafruit RP2040 Feather Board Support

**Branch**: `002-feather-rp2040-support` | **Date**: 2026-04-06 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-feather-rp2040-support/spec.md`

## Summary

Add a `feather_rp2040` PlatformIO build environment targeting the Adafruit RP2040 Feather +
Silicognition PoE FeatherWing. Simultaneously resolve an inherited gap: the W5500 Ethernet
chip has never been initialized in firmware (compile-only verification in 001). This feature
introduces board-specific W5500 init (build-flag-driven), a `NeoPixelStatusModule` with a
mockable `INeoPixelDriver` interface, updates both board example YAMLs to use their onboard
NeoPixels, and validates the Feather's GPIO pin set at config load time.

## Technical Context

**Language/Version**: C++14, Arduino framework (earlephilhower arduino-pico core)
**Primary Dependencies**:
  - PubSubClient (MQTT)
  - mobizt/ESP_SSLClient (TLS/W5500)
  - bblanchon/ArduinoJson (config parsing)
  - adafruit/Adafruit NeoPixel ← **new** (WS2812B via RP2040 PIO)
**Storage**: N/A (no new persistent storage; generated_config.h and LittleFS runtime config unchanged)
**Testing**: PlatformIO native (`pio test -e native`), Unity test framework, MockGPIODriver + new MockNeoPixelDriver
**Target Platform**: RP2040 (earlephilhower core) — two boards: `adafruit_feather_rp2040` + `silicognition_rp2040_shim`
**Project Type**: Embedded firmware framework extension
**Performance Goals**: NeoPixel update < 1ms (WS2812B PIO timing); Ethernet init < 5s on boot
**Constraints**: RP2040 264 KB SRAM — NeoPixelStatusModule flash + RAM footprint must be documented; no Wi-Fi dependency; SJIRQ solder jumper on PoE FeatherWing is default-open (polling mode, not interrupt mode)
**Scale/Scope**: 1 new build env, 1 new module, 2 new contracts, 1 new INeoPixelDriver interface, 2 updated example YAMLs, W5500 init added to both boards

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Offline-First Resilience | ✅ PASS | NeoPixel module is local I/O only; no network impact |
| II. Secure Transport | ✅ PASS | No changes to MQTT/TLS layer; `setInsecure()` fallback is dev-only and pre-existing |
| III. HA MQTT Discovery | ✅ PASS | NeoPixelStatusModule must publish HA Discovery (type: `light` or `sensor`); same pattern as existing modules |
| IV. YAML-Configurable | ✅ PASS | NeoPixel pin is YAML-configurable; W5500 SPI pins are hardware-fixed → driven by build env, not YAML; GPIO pin sets validated per board |
| V. Robust Testing | ✅ PASS | INeoPixelDriver abstraction enables MockNeoPixelDriver; unit tests required before merge |

No violations. No Complexity Tracking table required.

**Post-design re-check**: PASS — INeoPixelDriver interface satisfies Principle V; board-specific W5500 init via build flags satisfies Principle IV (YAML still drives all user-configurable behavior).

## Project Structure

### Documentation (this feature)

```text
specs/002-feather-rp2040-support/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output (Feather-specific supplement)
├── contracts/
│   ├── neopixel-driver-interface.md
│   └── board-environments.md
└── tasks.md             # Phase 2 output (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
platformio.ini                                # Add [env:feather_rp2040]

devices/
  feather-example.yml                         # New — Feather RP2040 reference config
  example.yml                                 # Update: status_led GPIO25 → neopixel_status GPIO23

include/
  INeoPixelDriver.h                           # New — mockable NeoPixel driver interface
  vanlab-iot.h                                # Update: include INeoPixelDriver + NeoPixelStatusModule

src/
  bundles/modules/neopixel/
    NeoPixelStatusModule.h                    # New — IModule impl; INeoPixelDriver-backed
    NeoPixelStatusModule.cpp                  # New
  hal/
    RP2040NeoPixelDriver.h                    # New — concrete INeoPixelDriver (Arduino only)
  main.cpp                                    # Update: board-specific W5500 init + NeoPixel module

tests/
  mocks/
    MockNeoPixelDriver.h                      # New — INeoPixelDriver test double
  test_neopixel_module/
    test_neopixel_module.cpp                  # New — NeoPixelStatusModule unit tests
```

**Structure Decision**: Single-project (existing vanlab-iot layout). New files follow the established `src/bundles/modules/<type>/` pattern. `INeoPixelDriver.h` lives in `include/` alongside `IGPIODriver.h`. Board-specific Ethernet init lives in `main.cpp` behind `#ifdef BOARD_FEATHER_RP2040` / `#ifdef BOARD_RP2040_SHIM` — these macros are set by `build_flags` in `platformio.ini` so no YAML change is required and the init is fully compile-time-determined.

## Hardware Reference (from research.md)

| Item | Feather RP2040 | RP2040-Shim |
|------|---------------|-------------|
| NeoPixel GPIO | 16 (internal, not on headers) | 23 |
| NeoPixel power enable | None | None |
| W5500 CS | GPIO 10 (D10) | GPIO 21 (D10 on Shim header) |
| W5500 INT | GPIO 12 (D12, SJIRQ open) | GPIO 14 (D12 on Shim header) |
| W5500 RST | RC circuit only (no GPIO) | RC circuit only (no GPIO) |
| SPI bus | SPI1 (SCK=18, MOSI=19, MISO=20) | SPI0 (SCK=10, MOSI=11, MISO=12) |
| Valid header GPIOs | 0,1,2,3,4,6,7,8,9,10,11,12,13,18,19,20,24,25,26,27,28,29 + internal 16 | Board-specific mapping |

**SJIRQ note**: The INT solder jumper on the PoE FeatherWing is default-open. The W5500 will be polled (loop-based) rather than interrupt-driven unless the jumper is bridged. This is acceptable for the use case and matches how PubSubClient/ESP_SSLClient work.

## Complexity Tracking

> Not required — all Constitution gates pass.
