# Contract: NeoPixel Standalone Module Type

**Feature**: 005-remove-rgb-light-env  
**Date**: 2026-04-18  
**Contract type**: YAML device config schema (standalone_modules[].type)

## Overview

After this feature, the NeoPixel operating mode is determined entirely by the `type` field of the first standalone module in the device YAML config. The build system no longer plays a role in mode selection.

## YAML Contract

```yaml
standalone_modules:
  - type: <neopixel-mode>   # Required. Determines NeoPixel operating mode.
    id: <string>            # Required. Unique module ID for MQTT topics.
    name: <string>          # Required. Human-readable name for HA device registry.
    gpio:
      pins: [<int>]         # Required. Single entry — the NeoPixel GPIO pin.
      roles: [indicator]    # Required. Must be "indicator" for NeoPixel modules.
```

## Valid `type` Values

| Value | Behavior | HA Entity Type |
|-------|----------|----------------|
| `neopixel_light` | Home Assistant-controllable RGB light. Subscribes to MQTT command topic. | `light` |
| `neopixel_status` | Connection state indicator. Green=connected, Yellow=connecting, Red=error. No HA entity. | none |

## Mode Selection Precedence

1. If `standalone_modules[0].type == "neopixel_light"` → NeoPixel light mode
2. All other values (including `"neopixel_status"`, missing, or null) → status indicator mode

## Example Configs

**NeoPixel as HA RGB light** (formerly required `feather_rgb_light` environment):
```yaml
standalone_modules:
  - type: neopixel_light
    id: rgb_light
    name: RGB Light
    gpio:
      pins: [16]        # onboard NeoPixel — Adafruit RP2040 Feather
      roles: [indicator]
```
Flash with: `pio run --target upload --environment feather_rp2040`

**NeoPixel as status indicator** (default behavior):
```yaml
standalone_modules:
  - type: neopixel_status
    id: neopixel_status
    name: Status
    gpio:
      pins: [16]        # onboard NeoPixel — Adafruit RP2040 Feather
      roles: [indicator]
```
Flash with: `pio run --target upload --environment feather_rp2040`

## Schema Version

No version bump required. The `type` field and its values already exist in the schema. This change documents the existing field as the authoritative mode selector, removing the build-flag workaround.
