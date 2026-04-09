# Tasks: Feather NeoPixel RGB Light

**Input**: Design documents from `/specs/003-feather-neopixel-light/`
**Prerequisites**: plan.md ✓, spec.md ✓, research.md ✓, data-model.md ✓, contracts/ ✓, quickstart.md ✓

**Organization**: Tasks grouped by user story for independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no incomplete dependencies)
- **[Story]**: User story (US1, US2, US3)

---

## Phase 1: Setup

**Purpose**: Reference device config for the RGB light device type.

- [X] T001 Create reference device config `devices/feather-rgb-light.yml` — board: adafruit_feather_rp2040, neopixel_light on GPIO 16, placeholder MQTT credentials (see data-model.md for full YAML)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Framework extension enabling standalone modules to subscribe to MQTT topics. Required before any user story can be implemented.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T002 Add `virtual void resubscribe(MQTTClientWrapper& mqtt) {}` default no-op to `IModule` class in `include/IModule.h` (forward-declare or include MQTTClientWrapper as needed; backward-compatible — all existing modules unaffected)
- [X] T003 Extend `BundleRegistry::resubscribeAll(MQTTClientWrapper& mqtt)` in `src/bundles/base/BundleRegistry.cpp` to iterate `_standaloneModules` and call `module->resubscribe(mqtt)` on each after the existing bundle loop

**Checkpoint**: IModule has resubscribe(). BundleRegistry calls it on all standalone modules on reconnect. User story work can now begin.

---

## Phase 3: User Story 1 — Register as Controllable RGB Light in HA (Priority: P1) 🎯 MVP

**Goal**: Flash the Feather, boot it, and a `light` entity appears in Home Assistant automatically — no manual HA configuration.

**Independent Test**: Flash the device with `feather-rgb-light` config. Observe in HA → Settings → Devices & Services → MQTT that a `light` entity appears within 30 seconds of boot. Verify it shows as available (not unavailable).

- [X] T004 [P] [US1] Create `NeoPixelLightModule` class declaration in `src/bundles/modules/neopixel/NeoPixelLightModule.h` — fields: `INeoPixelDriver& _driver`, `bool _isOn`, `uint8_t _r/_g/_b/_brightness`, topic char arrays `_commandTopic[128]/_stateTopic[128]/_discoveryTopic[128]`, `const char* _id/_name/_deviceId/_deviceName`; public methods: constructor, `setup()`, `loop()`, `resubscribe()`, `handleCommand(const char*, unsigned int)`, `getId()` (see data-model.md)
- [X] T005 [US1] Implement constructor, `setup()`, `getId()`, and `resubscribe()` in `src/bundles/modules/neopixel/NeoPixelLightModule.cpp` — constructor builds topic strings from `ModuleConfig.topic_root` + device fields; `setup()` calls `_driver.begin()`; `resubscribe()` subscribes to `_commandTopic` with QoS 1, publishes HA Discovery JSON (see data-model.md for payload schema) to `_discoveryTopic` retained, publishes current state JSON to `_stateTopic` retained (depends on T004)
- [X] T006 [P] [US1] Add `#include "src/bundles/modules/neopixel/NeoPixelLightModule.h"` (Arduino-gated) to `include/vanlab-iot.h`
- [X] T007 [P] [US1] Add `[env:feather_rgb_light]` to `platformio.ini` inheriting from `[env:feather_rp2040]` with `-DDEVICE_CONFIG_FILE='"devices/feather-rgb-light.yml"'` build flag
- [X] T008 [US1] Wire `NeoPixelLightModule` into `src/main.cpp` — declare `static RP2040NeoPixelDriver neoPixelDriver(pin)` and `static NeoPixelLightModule neoPixelLight(...)` when module type is `neopixel_light`; call `registry.registerModule(&neoPixelLight)` in `setup()` (depends on T004, T005)
- [X] T009 [P] [US1] Write Unity tests for constructor, `setup()` (driver.begin called), `getId()`, and `resubscribe()` (verify: MockMQTTClientWrapper receives subscribe call for correct command topic with QoS 1; discovery payload published to correct topic; state payload published) in `tests/test_neopixel_light/test_neopixel_light.cpp` — use `MockNeoPixelDriver` and a minimal `MockMQTTClientWrapper` stub (depends on T004, T005; parallel to T008)

**Checkpoint**: Device boots, connects to MQTT, HA light entity appears. Reboot → entity remains. All T009 tests pass.

---

## Phase 4: User Story 2 — Control NeoPixel from Home Assistant (Priority: P2)

**Goal**: HA user sends ON/OFF/color/brightness commands. NeoPixel responds within one second. HA reflects actual state.

**Independent Test**: From HA light card, send ON with red (R=255, G=0, B=0). NeoPixel turns red. Send OFF. NeoPixel turns off. HA state matches in both cases.

- [X] T010 [US2] Implement `handleCommand(const char* payload, unsigned int len)` in `src/bundles/modules/neopixel/NeoPixelLightModule.cpp` — parse JSON with ArduinoJson; apply state (ON/OFF), color (clamp R/G/B to [0,255]), brightness; ON without color → use last `_r/_g/_b` or default white (255,255,255); brightness-only while OFF → store, no NeoPixel change; ON → scale channels `(channel * _brightness + 127) / 255`, call `_driver.setColor(scaled_r, scaled_g, scaled_b)` + `_driver.show()`; call `publishState()` after any state change (depends on T005)
- [X] T011 [US2] Implement private `publishState()` helper in `src/bundles/modules/neopixel/NeoPixelLightModule.cpp` — serialize `{"state":"ON/OFF","color":{"r":...,"g":...,"b":...},"brightness":...}` and publish retained to `_stateTopic` (depends on T005)
- [X] T012 [US2] Set MQTT message callback in `src/main.cpp` — call `mqtt.setMessageCallback([](const char* topic, const uint8_t* payload, unsigned int len) { neoPixelLight.handleCommand((const char*)payload, len); })` after MQTT client init; ensure callback only routes messages matching `_commandTopic` (depends on T008, T010)
- [X] T013 [US2] Extend `tests/test_neopixel_light/test_neopixel_light.cpp` with `handleCommand` tests — ON with full red: driver shows R=255,G=0,B=0; OFF: driver shows 0,0,0, isOn false; ON after OFF (no color): restores last color; brightness scaling: ON with R=255 brightness=128 → driver gets ~128,0,0; ON without prior color (cold boot): uses white default; out-of-range R=300 clamped to 255; brightness-only while OFF: NeoPixel unchanged, brightness stored for next ON (depends on T009, T010, T011)

**Checkpoint**: HA can turn NeoPixel on/off, change color, adjust brightness. State reported back to HA. All T013 tests pass.

---

## Phase 5: User Story 3 — State Survives Disconnection and Reconnection (Priority: P3)

**Goal**: NeoPixel holds its color during outage. On reconnect, HA immediately reflects the correct state. HA commands sent while offline are applied on reconnect.

**Independent Test**: Command red. Disconnect network for 30 seconds — NeoPixel stays red. Reconnect — HA shows the correct ON/red state within 5 seconds without any user action.

- [X] T014 [P] [US3] Extend `tests/test_neopixel_light/test_neopixel_light.cpp` with reconnect/resilience tests — simulate reconnect by calling `resubscribe()` a second time while light is ON: verify discovery and state are republished; verify driver color is unchanged (NeoPixel not cleared by resubscribe); verify command topic re-subscribed with QoS 1 (broker-side offline buffering) (depends on T013)

**Checkpoint**: State retained across reconnects. HA re-syncs automatically. Offline commands (QoS 1 retained by broker) applied after reconnect. All T014 tests pass.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final verification and regression check.

- [X] T015 Run `pio test -e native` and confirm all existing tests plus new `test_neopixel_light` suite pass (zero regressions per FR-009)
- [X] T016 [P] Build `pio run -e feather_rgb_light` (or `feather_rp2040` with updated config flag) and confirm firmware compiles with zero errors using `devices/feather-rgb-light.yml`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: No dependencies — start immediately (can run in parallel with Phase 1)
- **User Stories (Phase 3+)**: Depend on Phase 2 completion
- **Polish (Phase 6)**: Depends on all user story phases complete

### User Story Dependencies

- **US1 (P1)**: Can start after Phase 2 (T002, T003)
- **US2 (P2)**: Depends on US1 (NeoPixelLightModule.cpp exists with setup/resubscribe) — specifically T005
- **US3 (P3)**: Depends on US2 (all handleCommand logic in place) — specifically T013

### Within Each User Story

- T004 (header) before T005 (impl) — then T008 (main.cpp wiring)
- T009 (tests) parallel to T008 (both read header T004)
- T010, T011 (handleCommand, publishState) before T012 (main.cpp callback) before T013 (tests)
- T014 (US3 tests) after T013

### Parallel Opportunities

- T001 (device YAML) can run parallel to T002/T003 (foundational)
- T004, T006, T007 can run in parallel (different files)
- T009 parallel to T008 (different files)

---

## Parallel Example: User Story 1

```
# These can run simultaneously (different files):
Task T004: NeoPixelLightModule.h header
Task T006: vanlab-iot.h include update
Task T007: platformio.ini new env

# After T004 completes, these can run simultaneously:
Task T005: NeoPixelLightModule.cpp (resubscribe/discovery)
Task T009: unit tests for setup/resubscribe
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 (T001) + Phase 2 (T002, T003) in parallel
2. Complete Phase 3 (T004–T009)
3. **STOP and VALIDATE**: Boot device, verify HA entity appears, run T009 tests
4. Feature has user-visible presence in HA — shippable MVP

### Incremental Delivery

1. Phase 1 + 2 → Framework extended
2. Phase 3 (US1) → HA entity visible → validate independently
3. Phase 4 (US2) → NeoPixel controllable → validate independently
4. Phase 5 (US3) → Reconnect resilience confirmed → validate independently
5. Phase 6 (Polish) → Full regression pass → ship

---

## Notes

- `handleCommand()` must remain MQTT-agnostic (no MQTTClientWrapper calls inside it) — testable with MockNeoPixelDriver only
- `MockMQTTClientWrapper` for T009: needs only `subscribe(topic, qos)` and `publish(topic, payload, retained)` recording — a minimal stub, not full mock
- GPIO 16 conflict: `neopixel_status` and `neopixel_light` are mutually exclusive in a device config — `feather-rgb-light.yml` must NOT include `neopixel_status`
- Brightness scaling uses integer arithmetic: `scaled = (channel * brightness + 127) / 255`
- Default color on first boot ON command: white (R=255, G=255, B=255), full brightness
