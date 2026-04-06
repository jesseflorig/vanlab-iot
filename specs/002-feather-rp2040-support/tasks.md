# Tasks: Adafruit RP2040 Feather Board Support

**Input**: Design documents from `/specs/002-feather-rp2040-support/`
**Prerequisites**: plan.md ✓, spec.md ✓, research.md ✓, data-model.md ✓, contracts/ ✓

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Exact file paths in all descriptions

---

## Phase 1: Setup

**Purpose**: PlatformIO environments and shared infrastructure

- [X] T001 Update platformio.ini — add `[env:feather_rp2040]` (platform: maxgerhardt raspberrypi, board: adafruit_feather_rp2040, board_build.core: earlephilhower, filesystem_size: 1m, lib_deps: PubSubClient + ESP_SSLClient + ArduinoJson + Adafruit NeoPixel, build_flags: -DBOARD_FEATHER_RP2040 + DEVICE_CONFIG_FILE feather-example.yml, extra_scripts: gen_config.py); also add `adafruit/Adafruit NeoPixel` to `[env:rp2040_shim]` lib_deps and add `-DBOARD_RP2040_SHIM` to its build_flags; add `[env:native]` build_src_filter exclusion for `src/hal/RP2040NeoPixelDriver.h` if it isn't already excluded by the ARDUINO guard in platformio.ini

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: INeoPixelDriver interface and mock — shared by US3 implementation and tests. Must be complete before Phase 5.

**⚠️ CRITICAL**: T002 must be complete before T003 (mock implements the interface)

- [X] T002 Create include/INeoPixelDriver.h — pure virtual interface: `virtual void begin() = 0`, `virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0`, `virtual void show() = 0`; virtual destructor; no includes beyond stdint.h; matches contract in contracts/neopixel-driver-interface.md
- [X] T003 Create tests/mocks/MockNeoPixelDriver.h — INeoPixelDriver implementation; public fields: `bool beginCalled = false`, `uint8_t lastR = 0`, `uint8_t lastG = 0`, `uint8_t lastB = 0`, `uint8_t showCount = 0`; implement all three methods recording calls; add `void reset()` that resets all fields to initial values

**Checkpoint**: Interface and mock ready — US3 implementation and tests can now proceed

---

## Phase 3: User Story 1 — Build Feather Firmware from YAML (Priority: P1) 🎯 MVP

**Goal**: `pio run -e feather_rp2040` compiles successfully with `devices/feather-example.yml`; invalid GPIO pins produce a boot-time error; no source edits required beyond copying the example YAML.

**Independent Test**: `pio run -e feather_rp2040` produces zero errors; `pio test -e native` still passes all existing tests.

- [X] T004 [US1] Create devices/feather-example.yml — reference config for Adafruit RP2040 Feather; set `board: adafruit_feather_rp2040`; no bundles section; standalone_modules with `type: neopixel_status`, id: neopixel_status, gpio.pins: [16], gpio.roles: [indicator]; include placeholder MQTT credentials with broker_host/port/client_id/keepalive_s/socket_timeout_s/topic_root fields
- [X] T005 [US1] Add `static bool validatePins(const DeviceConfig& cfg)` to src/config/ConfigLoader.h; implement in src/config/ConfigLoader.cpp — define `BOARD_VALID_PINS[]` array per board under `#ifdef BOARD_FEATHER_RP2040` (pins: 0,1,2,3,4,6,7,8,9,10,11,12,13,16,18,19,20,24,25,26,27,28,29) and `#elif defined(BOARD_RP2040_SHIM)` (use existing full range, or define conservatively); iterate all standalone_modules[i].gpio.pins[j] and bundles[b].modules[m].gpio.pins[j] where pin != -1; if pin not in BOARD_VALID_PINS, return false; wrap entire validatePins implementation in `#ifdef ARDUINO` since BOARD macros are only defined in Arduino builds; native build stub returns true
- [X] T006 [US1] Call `ConfigLoader::validatePins(DEVICE_CONFIG)` in setup() in src/main.cpp after `ConfigLoader::load()` — if validation fails, enter `while (true) delay(1000)` halt with a Serial.println error message

**Checkpoint**: `pio run -e feather_rp2040` compiles; a YAML with an invalid pin halts at boot with an error message

---

## Phase 4: User Story 2 — Ethernet Connectivity (Priority: P2)

**Goal**: W5500 on the PoE FeatherWing initializes on boot for both boards using board-specific SPI pin assignments from research.md.

**Independent Test**: Flash Feather device; observe serial output confirming W5500 initialization and IP acquisition within 30s of boot.

- [X] T007 [US2] Add board-specific W5500 initialization to src/main.cpp — at file scope add `#ifdef BOARD_FEATHER_RP2040 / static Wiznet5500lwIP eth(10, SPI1, 12); / #elif defined(BOARD_RP2040_SHIM) / static Wiznet5500lwIP eth(21, SPI0, 14); / #else / #error "No board macro defined..." / #endif`; add `static void setupSPI()` function with board-specific `SPI1.setRX(20); SPI1.setTX(19); SPI1.setSCK(18);` (Feather) or `SPI0.setRX(12); SPI0.setTX(11); SPI0.setSCK(10);` (Shim) behind the same ifdefs; call `setupSPI()` then `eth.begin()` at the top of setup() before anything else; add required includes (`<Ethernet.h>` or the lwIP W5500 header used by earlephilhower)

**Checkpoint**: Both board builds compile; W5500 init code is present and board-correct

---

## Phase 5: User Story 3 — NeoPixel Status Indicator (Priority: P3)

**Goal**: Onboard NeoPixel shows green (connected), yellow (connecting), red (error) on both boards; both example YAMLs pre-configure the correct NeoPixel pin; all native unit tests pass.

**Independent Test**: `pio test -e native` passes NeoPixelStatusModule tests; flash either board and observe NeoPixel color pattern matching connection state.

- [X] T008 [P] [US3] Create src/hal/RP2040NeoPixelDriver.h — concrete INeoPixelDriver; gate entire file in `#ifdef ARDUINO / #endif`; include `<Adafruit_NeoPixel.h>` and `INeoPixelDriver.h`; constructor `RP2040NeoPixelDriver(uint8_t pin)` stores `_pin`, constructs `Adafruit_NeoPixel _np{1, pin, NEO_GRB + NEO_KHZ800}`; implement begin() calling `_np.begin(); _np.clear(); _np.show();`; implement setColor(r,g,b) calling `_np.setPixelColor(0, r, g, b)`; implement show() calling `_np.show()`
- [X] T009 [US3] Create src/bundles/modules/neopixel/NeoPixelStatusModule.h — include IModule.h and INeoPixelDriver.h; declare `enum class StatusState { Connected, Connecting, Error }`; constructor `NeoPixelStatusModule(INeoPixelDriver& driver, const ModuleConfig& cfg, const DeviceConfig& dev)`; public `void setState(StatusState s)`; override setup(IGPIODriver&), loop(IGPIODriver&), publishDiscovery(MQTTClientWrapper&), publishState(MQTTClientWrapper&), getId(), getAvailabilityTopic(); private fields: `INeoPixelDriver& _driver`, `StatusState _state`, `uint32_t _lastToggleMs`, `bool _lit`, `const char* _id`, `const char* _availTopic`
- [X] T010 [US3] Create src/bundles/modules/neopixel/NeoPixelStatusModule.cpp — implement setup() calling `_driver.begin()`; implement loop() reading `gpio.millis()` for blink timer: Connected state toggles green(0,50,0)/off every 500ms; Connecting state toggles yellow(50,50,0)/off every 125ms; Error state always-on red(50,0,0) (no toggle); all state cases call `_driver.setColor(...)` then `_driver.show()`; implement publishDiscovery() and publishState() following StatusLEDModule.cpp pattern — module type string "neopixel_status"
- [X] T011 [US3] Write tests/test_neopixel_module/test_neopixel_module.cpp — Unity tests using MockNeoPixelDriver and a stub IGPIODriver (use existing MockGPIODriver from tests/mocks/); test: setup() calls begin() (mockDriver.beginCalled == true); setState(Connected) then loop() → lastR==0 && lastG>0 && lastB==0; setState(Connecting) then loop() → lastR>0 && lastG>0 && lastB==0; setState(Error) then loop() → lastR>0 && lastG==0 && lastB==0; Error state: showCount does not increase on repeated loop() ticks after initial show (no blinking in Error state)
- [X] T012 [P] [US3] Update include/vanlab-iot.h — add `#include "INeoPixelDriver.h"` in the Core interfaces section; add Arduino-gated `#include "../src/hal/RP2040NeoPixelDriver.h"`; add `#include "../src/bundles/modules/neopixel/NeoPixelStatusModule.h"` in the Standalone modules section
- [X] T013 [US3] Update src/main.cpp — replace `static StatusLEDModule statusLed(...)` with `static RP2040NeoPixelDriver neoPixelDriver(DEVICE_CONFIG.standalone_modules[0].gpio.pins[0])` and `static NeoPixelStatusModule neoPixelStatus(neoPixelDriver, DEVICE_CONFIG.standalone_modules[0], DEVICE_CONFIG)`; in setup() replace `registry.registerModule(&statusLed)` with `registry.registerModule(&neoPixelStatus)`; in loop() before `orchestrator.loop()` add a switch on `mqtt.state()` that calls `neoPixelStatus.setState(NeoPixelStatusModule::StatusState::Connected)` when `ConnState::Connected` and `setState(Connecting)` for Disconnected/Connecting/Backoff states
- [X] T014 [P] [US3] Update devices/example.yml — change standalone_modules entry: `type: status_led` → `type: neopixel_status`, id: `neopixel_status`, name: `"Status"`, `pins: [23]` (Shim onboard NeoPixel), roles: [indicator]; remove GPIO 25 pin reference

**Checkpoint**: All native tests pass including new NeoPixelStatusModule tests; both example YAMLs pre-configure the correct NeoPixel pin

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T015 Verify `pio test -e native` passes all tests (62 existing + new NeoPixelStatusModule tests); fix any native build failures caused by missing ARDUINO guard or include path issues in NeoPixelStatusModule or RP2040NeoPixelDriver
- [X] T016 Verify `pio run -e rp2040_shim` still compiles without regressions after all changes to main.cpp, ConfigLoader, platformio.ini, and devices/example.yml

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 — blocks Phase 5 (US3)
- **Phase 3 (US1)**: Depends on Phase 1 — independent of Phases 2, 4, 5
- **Phase 4 (US2)**: Depends on Phase 1 — independent of Phases 2, 3, 5
- **Phase 5 (US3)**: Depends on Phase 2 — independent of Phases 3, 4 except T013 (main.cpp) which should be done after T006 and T007 to avoid conflicts
- **Phase 6 (Polish)**: Depends on all phases complete

### User Story Dependencies

- **US1 (P1)**: Phase 1 complete → T004, T005 can run in parallel → T006 depends on T005
- **US2 (P2)**: Phase 1 complete → T007 (single task, standalone)
- **US3 (P3)**: Phase 2 complete → T008, T009 in parallel → T010 depends on T009 → T011 depends on T008+T009+T010 → T012, T013, T014 in parallel (different files)

### main.cpp Edit Order

T006, T007, and T013 all modify src/main.cpp. Complete them in order T006 → T007 → T013 to avoid edit conflicts.

---

## Parallel Opportunities

### Phase 3 (US1)
```
T004 (feather-example.yml) ─── parallel ─── T005 (ConfigLoader validatePins)
                                                   │
                                                  T006 (main.cpp call validatePins)
```

### Phase 5 (US3)
```
T008 (RP2040NeoPixelDriver) ─── parallel ─── T009 (NeoPixelStatusModule.h)
                                                   │
                                                  T010 (NeoPixelStatusModule.cpp)
                                                   │
T011 (unit tests) ─────────────────────────── depends on T008+T009+T010

T012 (vanlab-iot.h) ─── parallel ─── T013 (main.cpp) ─── parallel ─── T014 (example.yml)
(can start after T009)               (do last in phase, after T006+T007)
```

---

## Implementation Strategy

### MVP Scope: US1 Only (Build Verification)

1. Complete Phase 1 (T001)
2. Complete Phase 3 (T004, T005, T006)
3. `pio run -e feather_rp2040` — confirm zero errors
4. `pio test -e native` — confirm no regressions

### Full Delivery Order

1. T001 → platformio.ini
2. T002, T003 in parallel → INeoPixelDriver + Mock
3. T004, T005 in parallel (then T006) → US1
4. T007 → US2 W5500 init
5. T008, T009 in parallel (then T010, T011) → US3 core module
6. T012, T013, T014 in parallel (T013 after T006+T007) → US3 wiring
7. T015, T016 → validation

---

## Notes

- T013 is the most complex task — it touches main.cpp which also changed in T006 and T007; complete those first
- RP2040NeoPixelDriver.h is header-only and gated by `#ifdef ARDUINO`; no build_src_filter change needed for native builds
- NeoPixelStatusModule.cpp uses `gpio.millis()` — not `::millis()` — for testability; MockGPIODriver in tests/mocks/ must have millis() returning a controllable value (check MockGPIODriver.h before writing T011 tests)
- The module type string "neopixel_status" in NeoPixelStatusModule.cpp must match the YAML type field exactly
- `DEVICE_CONFIG.standalone_modules[0].gpio.pins[0]` in main.cpp assumes the NeoPixel module is always standalone_modules[0] — valid given both example YAMLs place it there
