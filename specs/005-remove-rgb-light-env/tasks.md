# Tasks: Remove Redundant feather_rgb_light Build Environment

**Input**: Design documents from `/specs/005-remove-rgb-light-env/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Core code changes are in `src/main.cpp`, `src/bundles/light/LightBundle.cpp`, and `platformio.ini`

---

## Phase 1: Setup

**Purpose**: Confirm baseline before any changes are made

- [X] T001 Run `pio test --environment native` from project root and confirm all 86 test cases across 10 suites pass before touching any code

---

## Phase 2: Foundational (Blocking Prerequisite)

**Purpose**: Make `LightBundle` safe to construct with an empty `BundleConfig` — required before `main.cpp` can always compile both module variants without an `#ifdef`

**⚠️ CRITICAL**: T003 and T004 in Phase 3 depend on this. If `LightBundle` crashes when `cfg.type == nullptr`, removing the `#ifdef` will cause a crash in neopixel_light configs that have no bundles.

- [X] T002 Add null-guard to `LightBundle` constructor in `src/bundles/light/LightBundle.cpp`: at the top of the constructor body, if `cfg.type == nullptr` (empty BundleConfig), skip all module initialization and return early — this prevents a crash when LightBundle is instantiated for a device that declares no bundles (neopixel_light configs)

**Checkpoint**: LightBundle can now be safely instantiated with a zeroed-out BundleConfig

---

## Phase 3: User Story 1 — Configure NeoPixel Mode via Device YAML (Priority: P1) 🎯 MVP

**Goal**: Remove the `NEOPIXEL_LIGHT_MODE` compile flag entirely. NeoPixel operating mode is determined at runtime by reading `DEVICE_CONFIG.standalone_modules[0].type` from the YAML-generated config.

**Independent Test**: Build `feather_rp2040` environment with `devices/feather-rgb-light.yml` (which has `type: neopixel_light`) and `devices/feather-example.yml` (which has `type: neopixel_status`) — both should compile without error and select the correct module path. Run native tests to confirm no regressions.

### Implementation for User Story 1

- [X] T003 [US1] Add `isNeoPixelLightMode()` static helper function to `src/main.cpp` immediately before the module declarations section. The function should return `true` when `DEVICE_CONFIG.standalone_module_count > 0 && DEVICE_CONFIG.standalone_modules[0].type != nullptr && strcmp(DEVICE_CONFIG.standalone_modules[0].type, "neopixel_light") == 0`, and `false` otherwise
- [X] T004 [US1] Replace all three `#ifdef NEOPIXEL_LIGHT_MODE` / `#ifndef NEOPIXEL_LIGHT_MODE` preprocessor blocks in `src/main.cpp` with runtime `if (isNeoPixelLightMode())` conditionals — both `NeoPixelLightModule` and (`LightBundle` + `NeoPixelStatusModule`) must always be statically declared and compiled in; only registration and loop logic branches at runtime
- [X] T005 [US1] Remove the entire `[env:feather_rgb_light]` section (lines 39–55) from `platformio.ini` — verify the remaining three environments (`rp2040_shim`, `feather_rp2040`, `native`) are intact and correctly formatted
- [X] T006 [US1] Run `pio test --environment native` and confirm all 86 test cases still pass with zero failures

**Checkpoint**: NeoPixel mode is now fully YAML-driven; `feather_rgb_light` environment is gone; native tests pass

---

## Phase 4: User Story 2 — Migrate Existing feather_rgb_light Device (Priority: P2)

**Goal**: The migration path is documented and a developer can reflash an existing `feather_rgb_light` device using `--environment feather_rp2040` without changing their device YAML.

**Independent Test**: Confirm `devices/feather-rgb-light.yml` has `type: neopixel_light` (it should — no edit needed). Confirm `specs/004-test-flash-feather/quickstart.md` no longer lists `feather_rgb_light` as a valid environment choice.

### Implementation for User Story 2

- [X] T007 [US2] Update `specs/004-test-flash-feather/quickstart.md`: remove the `feather_rgb_light` row from the environment table and remove the `feather_rgb_light` example in the "Flash firmware to Feather" section; replace with a note that NeoPixel mode is selected via YAML `type` field and only `feather_rp2040` is needed for both modes

**Checkpoint**: The migration guide is clear; no developer should reach for the now-deleted environment name

---

## Phase 5: User Story 3 — NeoPixel Light Mode Available on RP2040-Shim (Priority: P3)

**Goal**: Document that the RP2040-Shim can also run in NeoPixel light mode — a capability unlocked automatically by US1 with no additional code.

**Independent Test**: `devices/example.yml` correctly uses `type: neopixel_status`; a comment notes that `type: neopixel_light` is also valid for this board.

### Implementation for User Story 3

- [X] T008 [US3] Add a comment to `devices/example.yml` in the `standalone_modules` section noting that `type: neopixel_light` is also valid if the NeoPixel should be an HA-controllable RGB light instead of a status indicator — do not change the existing `type: neopixel_status` default

**Checkpoint**: Shim users know the option exists without requiring a separate environment

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Remove all remaining `feather_rgb_light` references from user-facing documentation

- [X] T009 [P] Update `README.md`: remove the `feather_rgb_light` row from the environment table in the Getting Started section (step 5); update the surrounding prose if it references `feather_rgb_light`
- [X] T010 [P] Update `CLAUDE.md`: remove the `feather_rgb_light` flash command line from the Commands section
- [X] T011 Run `pio test --environment native` one final time to confirm 86/86 pass after all changes

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 baseline confirmation — BLOCKS Phase 3
- **US1 (Phase 3)**: Depends on Phase 2 (T002 null-guard must exist before T004 removes the `#ifdef`)
- **US2 (Phase 4)**: Depends on Phase 3 completion (environment must be gone before migration docs are updated)
- **US3 (Phase 5)**: Depends on Phase 3 completion (NeoPixel light mode must work for both boards)
- **Polish (Phase 6)**: Depends on all story phases being complete

### Within Phase 3 (US1)

- T003 (add helper) must precede T004 (replace ifdefs) — same file, sequential
- T004 must precede T005 (remove environment) — logical dependency; code should work before env is removed
- T005 must precede T006 (test run) — final state must be verified together

### Parallel Opportunities

- T009 and T010 (Polish): Different files — can run in parallel
- T007 (US2) and T008 (US3): Different files — can run in parallel if US1 is complete

---

## Parallel Example: Polish Phase

```
# Launch both documentation cleanups together:
Task: "Remove feather_rgb_light from README.md environment table"
Task: "Remove feather_rgb_light from CLAUDE.md Commands section"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Baseline test run (T001)
2. Complete Phase 2: LightBundle null-guard (T002) — critical safety net
3. Complete Phase 3: US1 (T003 → T004 → T005 → T006) — core firmware change
4. **STOP and VALIDATE**: Native tests pass; `feather_rgb_light` environment is gone; both device YAMLs compile

### Incremental Delivery

1. Phase 1 + 2 → Safety net in place
2. Phase 3 (US1) → Core change done; NeoPixel mode is YAML-driven (MVP)
3. Phase 4 (US2) → Migration docs updated
4. Phase 5 (US3) → Shim capability documented
5. Phase 6 (Polish) → All references cleaned up; final test run

---

## Notes

- [P] tasks touch different files — no conflicts
- T002 (null-guard) is the only risk item — if LightBundle's constructor is already safe for null cfg, T002 may be a no-op add; read the file to confirm before editing
- T003 and T004 are in the same file (`src/main.cpp`) — do them sequentially, not in parallel
- T001 and T011 are the correctness bookends — both should show 86/86
- `devices/feather-rgb-light.yml` requires no changes — it already has `type: neopixel_light`
- Commit after Phase 3 is confirmed green (T006 passes) before proceeding to docs cleanup
