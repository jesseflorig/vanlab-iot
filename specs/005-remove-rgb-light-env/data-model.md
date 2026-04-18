# Data Model: Remove Redundant feather_rgb_light Build Environment

**Feature**: 005-remove-rgb-light-env  
**Date**: 2026-04-18

## NeoPixel Mode — Runtime Type Selection

This feature introduces no new data entities. It repurposes the existing `ModuleConfig.type` field — already present in the codebase — as the sole source of truth for NeoPixel operating mode.

### ModuleConfig.type — NeoPixel Semantics

| Value | Mode | Behavior |
|-------|------|----------|
| `"neopixel_light"` | HA RGB Light | NeoPixelLightModule registered; subscribes to MQTT command topic; publishes HA `light` discovery |
| `"neopixel_status"` | Status Indicator | NeoPixelStatusModule registered; reflects MQTT connection state (green=connected, yellow=connecting, red=error) |

**Default** (unspecified or unrecognized): treated as `"neopixel_status"` — `isNeoPixelLightMode()` returns false for any type string that is not exactly `"neopixel_light"`.

### State Transitions

```
firmware boot
    │
    ▼
read DEVICE_CONFIG.standalone_modules[0].type
    │
    ├─ "neopixel_light" ──▶ register NeoPixelLightModule
    │                            │
    │                            ▼
    │                       await MQTT command → update NeoPixel
    │
    └─ anything else ─────▶ register LightBundle + NeoPixelStatusModule
                                 │
                                 ▼
                            reflect MQTT ConnState → update NeoPixel each loop tick
```

### Validation Rules

- `DEVICE_CONFIG.standalone_module_count > 0` must be true for any NeoPixel mode to activate
- `DEVICE_CONFIG.standalone_modules[0].type` must be non-null
- An unrecognized type value is silently treated as status mode (fail-safe default)
- An invalid mode value does **not** halt startup — the firmware continues with status mode

> **Note**: FR-006 in the spec calls for a config validation error on unrecognized mode values. This is aspirational. Given the embedded fail-safe constraint, the practical implementation defaults to status mode and logs a warning rather than halting. A hard halt would brick a remote device.
