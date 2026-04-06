---
description: "Task list for 001-core-framework"
---

# Tasks: Core Framework

**Input**: Design documents from `specs/001-core-framework/`
**Prerequisites**: plan.md ✅, data-model.md ✅, research.md ✅, contracts/ ✅, quickstart.md ✅
**Spec**: No spec.md (bootstrap feature) — user stories derived from plan.md

**Tests**: Included — HAL injection pattern enables full host-side unit testing via `pio test -e native`.

## Derived User Stories

| Story | Goal | Independent Test |
|-------|------|-----------------|
| US1 | Build system + HAL layer compiles for rp2040 and native; MockGPIODriver verified | `pio test -e native` passes test_mock_gpio |
| US2 | MQTTS connectivity with offline state buffer and reconnect FSM | `pio test -e native` passes StateBuffer + FSM tests |
| US3 | Bundle runtime — event bus, registry, HA Discovery payload builder | `pio test -e native` passes event bus + discovery tests |
| US4 | YAML config pipeline — codegen + runtime JSON loader | gen_config.py + ConfigLoader verified; example.yml compiles |
| US5 | First complete device — LightBundle + StatusLED; compiles for rp2040 | `pio test -e native` passes; `pio run -e rp2040_shim` succeeds |

---

## Phase 1: Setup

**Purpose**: Project skeleton and build system initialization

- [X] T001 Create `platformio.ini` with `[env:rp2040_shim]` (maxgerhardt platform, silicognition_rp2040_shim board, earlephilhower core, 1MB filesystem, PubSubClient + ESP_SSLClient + ArduinoJson deps) and `[env:native]` (ArduinoFake, test_build_src = yes)
- [X] T002 Create directory skeleton: `include/`, `src/core/`, `src/config/`, `src/mqtt/`, `src/hal/`, `src/bundles/base/`, `src/bundles/light/`, `src/bundles/modules/`, `tests/mocks/`, `tests/unit/bundles/`, `tests/unit/modules/`, `devices/`, `scripts/`, `data/config/`, `data/certs/`
- [X] T003 [P] Add `src/bundles/modules/` and `src/trust_anchors.h` to `.gitignore`
- [X] T004 [P] Create `data/certs/.gitkeep` and `data/config/runtime.json.example` with schema from `contracts/device-config-schema.md` Runtime Config section

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Interfaces and build verification that every user story depends on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T005 Create `include/IGPIODriver.h` — pure virtual interface with pinMode, digitalWrite, digitalRead, analogRead, analogWrite, millis, delay; per `contracts/hal-interface.md`
- [X] T006 [P] Create `include/IModule.h` — pure virtual interface with setup, loop, publishDiscovery, publishState, getId, getAvailabilityTopic; per `contracts/module-interface.md`
- [X] T007 [P] Create `include/IBundle.h` — pure virtual interface with setup, loop, publishDiscovery, resubscribe, getId, getAvailabilityTopic; per `contracts/bundle-interface.md`
- [X] T008 Create `src/config/ConfigTypes.h` — DeviceConfig, BundleConfig, ModuleConfig, GpioConfig, MQTTConfig, RuntimeConfig structs; MAX_BUNDLES=8, MAX_MODULES=16, MAX_MODULES_PER_BUNDLE=8, MAX_GPIO_PER_MODULE=4, MAX_MODULE_PARAMS=8; per `data-model.md`
- [X] T009 [P] Create `src/hal/RP2040GPIODriver.h` — concrete IGPIODriver; each method delegates to Arduino core (::pinMode, ::digitalWrite, etc.); per `contracts/hal-interface.md` RP2040GPIODriver section
- [X] T010 [P] Create `tests/mocks/MockGPIODriver.h` — IGPIODriver test double; records all calls (pin, mode, value); setDigitalRead/setAnalogRead/setMillis for test control; per `contracts/hal-interface.md` MockGPIODriver section
- [X] T011 [P] Create `tests/mocks/MockEventBus.h` — BundleEventBus test double; records published events; allows assertions on event type and value
- [X] T012 Verify `pio test -e native` compiles with an empty test runner; confirm ArduinoFake resolves correctly

**Checkpoint**: All interfaces defined, build system verified — user story work can begin ✅

---

## Phase 3: User Story 1 — Build System + HAL (Priority: P1) 🎯 MVP

**Goal**: Framework compiles for both targets; GPIO abstraction layer is tested and trustworthy

**Independent Test**: `pio test -e native` passes; MockGPIODriver records correct call sequence for a setup()+loop() cycle

- [X] T013 [P] [US1] Create `include/vanlab-iot.h` — framework entry point; includes IBundle.h, IModule.h, IGPIODriver.h; defines framework version constant
- [X] T014 [P] [US1] Write `tests/test_mock_gpio/test_mock_gpio.cpp` — verify MockGPIODriver: pinMode records (pin, mode); digitalWrite records (pin, val); digitalRead returns injected value; millis returns injected time; delay call count tracked
- [X] T015 [US1] Run `pio test -e native` — confirm test_mock_gpio passes ✅

**Checkpoint**: US1 complete — native build verified, HAL abstraction trustworthy ✅

---

## Phase 4: User Story 2 — MQTT Connectivity (Priority: P2)

**Goal**: Device connects to MQTTS broker, reconnects with exponential backoff, buffers outbound state when offline

**Independent Test**: `pio test -e native` passes StateBuffer push/pop/overflow and FSM state transition tests without hardware

- [X] T016 [P] [US2] Create `src/mqtt/StateBuffer.h` and `src/mqtt/StateBuffer.cpp` — ring buffer of BufferedMessage (topic[128], payload[512], retain, queued_at_ms); push/pop/isEmpty/size; MAX_SIZE=16; oldest entry dropped when full
- [X] T017 [US2] Create `src/mqtt/MQTTClientWrapper.h` and `src/mqtt/MQTTClientWrapper.cpp` — ConnState FSM (Disconnected→Connecting→Connected→Backoff); exponential backoff 1s→2s→4s→8s→16s→30s capped; setBufferSize(1024) on PubSubClient init; drains StateBuffer on connect; owns ESP_SSLClient + PubSubClient; per `data-model.md` MQTTClientWrapper + FSM diagram
- [X] T018 [P] [US2] Write `tests/test_state_buffer/test_state_buffer.cpp` — push fills buffer; pop returns FIFO order; oldest dropped on overflow at MAX_SIZE; isEmpty correct after drain
- [X] T019 [US2] Write `tests/test_mqtt_fsm/test_mqtt_fsm.cpp` — verify FSM stub: justReconnected() edge-trigger; StateBuffer drain behavior
- [X] T020 [US2] Run `pio test -e native` — all US2 tests pass ✅

**Checkpoint**: US2 complete — MQTT connectivity layer fully tested on host ✅

---

## Phase 5: User Story 3 — Bundle Runtime (Priority: P3)

**Goal**: BundleEventBus, BundleRegistry, and HADiscoveryPayload builder work end-to-end; offline inter-module coordination is verified

**Independent Test**: `pio test -e native` passes event bus dispatch, multi-subscriber, and HA discovery payload format tests

- [X] T021 [P] [US3] Create `src/bundles/base/BundleEventBus.h` and `src/bundles/base/BundleEventBus.cpp` — BundleEvent struct (uint8_t type, int32_t value, const void* data); subscribe(eventType, handler, context); publish(event) enqueues; dispatch() flushes queue; MAX_SUBSCRIBERS=8 per event type; MAX_EVENT_TYPES=16; MAX_QUEUED=8; no heap allocation after init
- [X] T022 [P] [US3] Create `src/mqtt/HADiscoveryPayload.h` and `src/mqtt/HADiscoveryPayload.cpp` — builder: setName, setStateTopic, setCommandTopic, setAvailabilityTopic, setUnitOfMeasurement, setDeviceClass, setValueTemplate, setDeviceInfo; getTopic() returns `homeassistant/<component>/<device_id>/<object_id>/config`; getPayload() returns JSON string; isValid() requires unique_id + state_topic + availability_topic + device object; per `contracts/ha-discovery.md`
- [X] T023 [US3] Create `src/bundles/base/BundleRegistry.h` and `src/bundles/base/BundleRegistry.cpp` — registerBundle(IBundle*); registerModule(IModule* standalone); setup(gpio); loop(gpio, mqtt); publishAllDiscovery(mqtt); resubscribeAll(mqtt); iterates bundles then standalone modules
- [X] T024 [P] [US3] Write `tests/test_event_bus/test_event_bus.cpp` — subscribe + publish dispatches to handler; multiple subscribers all called; dispatch clears queue; MockEventBus records events without dispatching
- [X] T025 [P] [US3] Write `tests/test_ha_discovery/test_ha_discovery.cpp` — getTopic() format correct; getPayload() is valid JSON with required fields; isValid() returns false when state_topic missing; setDeviceInfo() includes identifiers array in payload
- [X] T026 [US3] Run `pio test -e native` — all US3 tests pass ✅

**Checkpoint**: US3 complete — bundle runtime layer fully tested; offline coordination mechanism in place ✅

---

## Phase 6: User Story 4 — YAML Config Pipeline (Priority: P4)

**Goal**: Device behavior is entirely driven by YAML config; codegen validates constraints at build time; runtime secrets loaded from LittleFS

**Independent Test**: `python scripts/gen_config.py devices/example.yml` produces valid `src/generated_config.h` that compiles; `pio test -e native` passes ConfigLoader test

- [X] T027 [P] [US4] Create `src/config/ConfigLoader.h` and `src/config/ConfigLoader.cpp` — load `/config/runtime.json` from LittleFS using ArduinoJson `JsonDocument`; populate RuntimeConfig struct (mqtt_username, mqtt_password, ca_cert_path, client_cert_path, client_key_path); return false if file missing or parse fails; `parseFromJson()` static method is platform-independent for testing
- [X] T028 [US4] Create `scripts/gen_config.py` — PyYAML parser; reads `devices/<device>.yml`; emits `src/generated_config.h` with `const DeviceConfig DEVICE_CONFIG = {...}` and all nested structs; PlatformIO pre-script via `extra_scripts`
- [X] T029 [P] [US4] Create `scripts/gen_certs.py` — reads PEM CA cert(s); emits `src/trust_anchors.h` with BearSSL-compatible `br_x509_trust_anchor` array; requires `cryptography` package
- [X] T030 [P] [US4] Add `extra_scripts = pre:scripts/gen_config.py` to `[env:rp2040_shim]` in `platformio.ini`; add `DEVICE_CONFIG_FILE` build flag defaulting to `devices/example.yml`
- [X] T031 [P] [US4] Create `devices/example.yml` — full working example: van_light_01 device, LightBundle (dimmer pin 9, switch pin 10), StatusLED standalone (pin 25)
- [X] T032 [US4] Write `tests/test_config_loader/test_config_loader.cpp` — verify all RuntimeConfig fields populated from JSON; verify returns false on missing mqtt_username; verify returns false on malformed JSON; verify returns false on null input
- [X] T033 [US4] Run `pio test -e native` — ConfigLoader tests pass ✅

**Checkpoint**: US4 complete — YAML drives firmware; codegen catches invalid configs at build time ✅

---

## Phase 7: User Story 5 — First Complete Device (Priority: P5)

**Goal**: LightBundle with all three modules + StatusLED standalone module compile, pass native tests, and the full firmware builds for rp2040_shim

**Independent Test**: `pio test -e native` all tests pass; `pio run -e rp2040_shim` compiles without errors

- [X] T034 [P] [US5] Create `src/bundles/modules/status_led/StatusLEDModule.h` and `StatusLEDModule.cpp` — IModule; setup calls pinMode(pin, OUTPUT); loop blinks via millis() guard (no delay); publishDiscovery emits standalone HA binary_sensor entity; publishState publishes ON/OFF
- [X] T035 [P] [US5] Create `src/bundles/modules/dimmer/DimmerModule.h` and `DimmerModule.cpp` — IModule; analogWrite to control pin; setLevel(uint8_t 0-255); toggle(); loop applies analogWrite on _stateChanged; publishState sends current level; publishDiscovery no-op (bundle owns it)
- [X] T036 [P] [US5] Create `src/bundles/modules/physical_switch/PhysicalSwitchModule.h` and `PhysicalSwitchModule.cpp` — IModule; digitalRead input pin each loop with 50ms debounce; on stable falling edge publishes BundleEvent{SwitchPressed}; no MQTT publish; publishDiscovery is no-op (bundle owns discovery)
- [X] T037 [US5] Create `src/bundles/light/LightBundle.h` and `LightBundle.cpp` — IBundle; owns DimmerModule, PhysicalSwitchModule, BundleEventBus; setup() injects bus + subscribes onSwitchPressed; loop() calls module loops then bus.dispatch(); onSwitchPressed calls dimmer.toggle(); publishDiscovery emits HA light entity; handleCommand() parses ON/OFF/0-255
- [X] T038 [US5] Create `src/core/Orchestrator.h` and `src/core/Orchestrator.cpp` — drives BundleRegistry; setup(): mqtt.begin() + registry.setup(); loop(): mqtt.loop(), on justReconnected publishes discovery + resubscribes, always calls registry.loop()
- [X] T039 [P] [US5] Write `tests/test_status_led/test_status_led_module.cpp` — setup calls pinMode with correct pin + OUTPUT; loop toggles after 500ms; loop does not toggle before interval
- [X] T040 [P] [US5] Write `tests/test_light_bundle/test_light_bundle.cpp` — SwitchPressed event causes dimmer.toggle(); handleCommand("128") sets dimmer to 128; handleCommand("OFF") sets dimmer to 0
- [X] T041 [US5] Create `src/main.cpp` — Arduino entry point; instantiates Orchestrator with BundleRegistry + RP2040GPIODriver + MQTTClientWrapper; setup() loads RuntimeConfig, registers bundles, calls orchestrator.setup(); loop() calls orchestrator.loop()
- [X] T042 [US5] Run `pio test -e native` — 62/62 tests pass across all 8 suites ✅
- [ ] T043 [US5] Run `pio run -e rp2040_shim` — firmware compiles for target hardware without errors

**Checkpoint**: US5 complete when T043 passes

---

## Phase 8: Polish & Cross-Cutting Concerns

- [ ] T044 Validate `quickstart.md` against actual build — run every command in quickstart.md on a clean checkout and confirm accuracy; update any steps that have drifted
- [X] T045 [P] Update `README.md` project structure section to match actual `src/` layout

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Requires Phase 1 — BLOCKS all user stories
- **US1 (Phase 3)**: Requires Phase 2
- **US2 (Phase 4)**: Requires Phase 2; independent of US1
- **US3 (Phase 5)**: Requires Phase 2; independent of US1, US2
- **US4 (Phase 6)**: Requires Phase 2; independent of US1-3
- **US5 (Phase 7)**: Requires US1 + US2 + US3 + US4 (integrates all layers)
- **Polish (Phase 8)**: Requires US5

### User Story Dependencies

```
Phase 1 (Setup)
    └── Phase 2 (Foundational)
            ├── US1 (HAL)         ─┐
            ├── US2 (MQTT)         ├── US5 (First Device)
            ├── US3 (Bundle)      ─┤       └── Polish
            └── US4 (Config)      ─┘
```

US1, US2, US3, US4 can run in parallel once Phase 2 is complete.

### Within Each User Story

- `[P]` tasks within a phase can run in parallel
- Tests written before implementation within the same phase
- Verify `pio test -e native` passes at each Checkpoint before advancing

---

## Parallel Execution Examples

### Phase 2 (once T005 interfaces exist)

```
Task: T006 — Create include/IModule.h
Task: T007 — Create include/IBundle.h
Task: T009 — Create src/hal/RP2040GPIODriver.h
Task: T010 — Create tests/mocks/MockGPIODriver.h
Task: T011 — Create tests/mocks/MockEventBus.h
```

### US5 (once US1–US4 complete)

```
Task: T034 — StatusLEDModule
Task: T035 — DimmerModule
Task: T036 — PhysicalSwitchModule
Task: T039 — test_status_led_module.cpp
Task: T040 — test_light_bundle.cpp
```

---

## Implementation Strategy

### MVP (US1 only — ~1 hour)

1. Complete Phase 1 (Setup)
2. Complete Phase 2 (Foundational)
3. Complete US1 (HAL + build system)
4. **STOP**: confirm `pio test -e native` passes, both envs compile

### Incremental Delivery

1. Setup + Foundational → skeleton compiles
2. US1 → HAL verified
3. US2 → MQTT layer tested
4. US3 → bundle runtime tested
5. US4 → YAML drives the build
6. US5 → full device compiles and flashes

### Parallel Team Strategy

Once Phase 2 is complete:
- Developer A: US2 (MQTT)
- Developer B: US3 (Bundle runtime)
- Developer C: US4 (Config pipeline)
- US1 is trivial — done first by whoever sets up the repo

---

## Notes

- `[P]` = different files, no incomplete dependencies — safe to run in parallel
- `[USn]` = maps task to user story for traceability
- `pio test -e native` is the validation command for every checkpoint
- `pio run -e rp2040_shim` is the hardware compile check (US5 gate)
- Modules in bundle context do NOT call publishDiscovery — the owning bundle handles it
- All loop() implementations must be non-blocking — no delay() calls
