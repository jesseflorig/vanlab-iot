# Tasks: Validate Local Testing and Flashing to RP2040 Feather via USB-C

**Input**: Design documents from `/specs/004-test-flash-feather/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, quickstart.md ✅

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- This feature is documentation-only — all implementation tasks are edits to `README.md` and `CLAUDE.md`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm baseline project state before making any changes

- [X] T001 Verify PlatformIO CLI is installed and `pio` is on PATH (`pio --version`)

---

## Phase 2: Foundational (Blocking Prerequisite)

**Purpose**: Confirm the native test suite runs and passes before documenting it

**⚠️ CRITICAL**: Establishes the baseline. If tests fail, diagnose and fix before proceeding to documentation.

- [X] T002 Run `pio test --environment native` from the project root and confirm all 10 test suites pass with zero failures

**Checkpoint**: Native tests pass — safe to document the workflow

---

## Phase 3: User Story 1 — Run Native Unit Tests Locally (Priority: P1) 🎯 MVP

**Goal**: Developer can run the native test suite locally and get a clear pass/fail result without hardware attached.

**Independent Test**: Run `pio test --environment native` and observe a pass/fail count in terminal output for all suites.

### Implementation for User Story 1

- [X] T003 [P] [US1] Update `README.md` Getting Started section: add step 2a `pio test --environment native` with a brief description of what it tests and why (before the flash step)
- [X] T004 [P] [US1] Update `CLAUDE.md` Commands section: replace the `# Add commands for C++14, Arduino framework` placeholder with the canonical `pio test --environment native` command and a one-line description

**Checkpoint**: A developer reading README.md or CLAUDE.md can now discover and run native tests independently

---

## Phase 4: User Story 2 — Flash Firmware to Feather via USB-C (Priority: P2)

**Goal**: Developer can flash firmware to a connected Feather using a correct, environment-specific command.

**Independent Test**: Following the documented steps flashes the board successfully (or produces a clear error if the board isn't connected).

### Implementation for User Story 2

- [X] T005 [US2] Fix `README.md` Getting Started flash step: replace `--environment rp2040` (non-existent) with environment options `feather_rp2040` and `feather_rgb_light`, with a brief note on when to choose each
- [X] T006 [P] [US2] Update `CLAUDE.md` Commands section: add `pio run --target upload --environment feather_rp2040` and `pio run --target upload --environment feather_rgb_light` with one-line descriptions of each

**Checkpoint**: README and CLAUDE.md both reference valid environment names; the README bug is resolved

---

## Phase 5: User Story 3 — Confirm Serial Monitor Output After Flash (Priority: P3)

**Goal**: Developer knows the serial monitor command and expected baud rate after a flash.

**Independent Test**: Running `pio device monitor` after a flash shows firmware startup output.

### Implementation for User Story 3

- [X] T007 [P] [US3] Review and confirm the `README.md` Getting Started serial monitor step uses `pio device monitor` (verify it's accurate; update if it references flags or a specific port unnecessarily)
- [X] T008 [P] [US3] Update `CLAUDE.md` Commands section: add `pio device monitor` with a note that PlatformIO auto-detects the port and uses 115200 baud as configured in `platformio.ini`

**Checkpoint**: All three user story workflows are documented in both README.md and CLAUDE.md

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Consistency pass across all changed documentation

- [X] T009 Read `README.md` Getting Started section end-to-end and verify all four steps (test, flash feather_rp2040, flash feather_rgb_light, monitor) form a coherent workflow with no contradictions
- [X] T010 [P] Read `CLAUDE.md` Commands section end-to-end and confirm all commands are present, accurate, and consistently formatted
- [X] T011 Run `pio test --environment native` one final time to confirm the documented command matches what actually works

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Phase 1
- **User Stories (Phase 3–5)**: All depend on Phase 2 completion; US stories can proceed in any order after that
- **Polish (Phase 6)**: Depends on all story phases being complete

### User Story Dependencies

- **US1 (P1)**: Starts after Phase 2; no dependency on US2 or US3
- **US2 (P2)**: Starts after Phase 2; no dependency on US1 or US3
- **US3 (P3)**: Starts after Phase 2; no dependency on US1 or US2
- US1, US2, and US3 all edit different sections of the same two files — sequence them to avoid conflicts

### Within Each User Story

- `README.md` and `CLAUDE.md` edits for the same story are marked [P] (different files, no conflicts)
- Each story's README task must complete before the Polish consistency read in Phase 6

### Parallel Opportunities

- T003 and T004 (US1): Different files — can run in parallel
- T005 and T006 (US2): Different files — can run in parallel
- T007 and T008 (US3): Different files — can run in parallel
- T009 and T010 (Polish): Different files — can run in parallel

---

## Parallel Example: User Story 2

```
# Launch both US2 edits in parallel (different files):
Task: "Fix README.md Getting Started flash step with correct environment names"
Task: "Update CLAUDE.md Commands section with feather flash commands"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001)
2. Complete Phase 2: Foundational (T002) — confirm tests pass
3. Complete Phase 3: US1 (T003, T004) — document native test command
4. **STOP and VALIDATE**: Developer can find and run `pio test --environment native` from the docs

### Incremental Delivery

1. Phase 1 + 2 → Baseline confirmed
2. US1 (T003–T004) → Native test workflow documented (MVP)
3. US2 (T005–T006) → Flash workflow corrected and documented
4. US3 (T007–T008) → Serial monitor workflow documented
5. Phase 6 (T009–T011) → Full consistency pass + final test run

---

## Notes

- [P] tasks touch different files — no conflicts
- This is documentation-only; no src/ or tests/ changes
- T002 and T011 (native test runs) are the primary correctness checks
- If T002 reveals a failing test, fix the underlying test issue before proceeding — do not document a broken workflow
- Commit after completing each user story phase or after Polish
