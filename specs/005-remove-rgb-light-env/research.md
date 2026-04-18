# Research: Remove Redundant feather_rgb_light Build Environment

**Feature**: 005-remove-rgb-light-env  
**Date**: 2026-04-18

## Findings

### The Redundancy Is Real — and Already Half-Solved

**Decision**: Remove `feather_rgb_light` from `platformio.ini` and replace the `#ifdef NEOPIXEL_LIGHT_MODE` blocks in `main.cpp` with a runtime type check on `DEVICE_CONFIG.standalone_modules[0].type`.

**Rationale**: The YAML `type` field on the first standalone module is already `"neopixel_light"` or `"neopixel_status"`. `gen_config.py` already captures this string in `DEVICE_CONFIG.standalone_modules[0].type`. The only thing that reads `NEOPIXEL_LIGHT_MODE` is `main.cpp` (three `#ifdef` / `#ifndef` sites). The flag is entirely superseded by what's already in the config.

**Alternatives considered**:
- Keep the compile flag and add a YAML validator that enforces consistency — rejected because it's two sources of truth for the same decision, which is exactly the problem being fixed.
- Use `constexpr` string comparison to select at compile time — not possible; `DEVICE_CONFIG` is not `constexpr`.
- Use C++ `std::variant` or `std::optional` (C++17) for conditional module allocation — rejected; project is C++14 and heap-free embedded code.

### Runtime Mode Detection Pattern

```cpp
static bool isNeoPixelLightMode() {
    return DEVICE_CONFIG.standalone_module_count > 0
        && DEVICE_CONFIG.standalone_modules[0].type != nullptr
        && strcmp(DEVICE_CONFIG.standalone_modules[0].type, "neopixel_light") == 0;
}
```

Both module variants (`NeoPixelLightModule` and `NeoPixelStatusModule`) are always compiled in. Memory is statically allocated for both. The active one is selected in `setup()` and `loop()` via `isNeoPixelLightMode()`. For the RP2040's 264 KB SRAM, the marginal cost of carrying both objects is negligible.

### Static Initialization Risk: LightBundle With Empty BundleConfig

**Finding**: When the YAML has no `bundles:` section (e.g., `feather-rgb-light.yml`), `DEVICE_CONFIG.bundle_count = 0` and `DEVICE_CONFIG.bundles[0]` is zero-initialized (all pointers null, counts zero). Today, the `#ifdef` prevents `LightBundle` from ever being constructed in this case. After removing the `#ifdef`, `LightBundle` is always constructed — it must handle a null/empty `BundleConfig` without crashing.

**Decision**: Add a null-guard check at the top of the `LightBundle` constructor: if `cfg.type == nullptr`, skip module initialization (treat as no-op / empty bundle). The bundle will never be registered in this case, so setup/loop are never called — but the constructor must not dereference nulls.

**Alternatives considered**:
- Sentinel BundleConfig with empty strings instead of nullptrs — would require gen_config.py changes and is more fragile.
- Check `DEVICE_CONFIG.bundle_count > 0` before constructing `LightBundle` — not possible for C++ static globals (constructed before main).

### YAML Config Files: No Changes Required

**Finding**: `devices/feather-rgb-light.yml` already uses `type: neopixel_light` on its standalone module. It requires no changes — it will work identically with the `feather_rp2040` environment. The only user action needed is switching `--environment feather_rgb_light` → `--environment feather_rp2040` in their flash command.

### gen_config.py: No Changes Required

**Finding**: `gen_config.py` already captures and emits the `type` string for every module. The runtime check in `main.cpp` reads what is already there.

### YAML Schema: `neopixel_light` Is Already Valid

**Finding**: The module type string `"neopixel_light"` is already recognized by the firmware module registration path. No schema version bump is required because no field is added, removed, or redefined — only the build system's use of a compile flag is removed. The schema is unchanged.

### Documentation Surfaces to Update

1. `platformio.ini` — remove `[env:feather_rgb_light]` section
2. `src/main.cpp` — replace three `#ifdef`/`#ifndef NEOPIXEL_LIGHT_MODE` blocks
3. `src/bundles/light/LightBundle.cpp` (or `.h`) — add null-guard for empty BundleConfig
4. `README.md` — remove `feather_rgb_light` from environment table in Getting Started
5. `CLAUDE.md` — remove `feather_rgb_light` from Commands section
6. `specs/004-test-flash-feather/quickstart.md` — remove `feather_rgb_light` from environment table
