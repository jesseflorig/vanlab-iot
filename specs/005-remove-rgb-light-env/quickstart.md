# Quickstart: NeoPixel Mode via YAML (Post-Migration)

**Feature**: 005-remove-rgb-light-env  
**Date**: 2026-04-18

## What Changed

The `feather_rgb_light` build environment is removed. NeoPixel operating mode is now selected entirely by the `type` field in the device YAML config. No build environment change is needed to switch between status indicator and HA RGB light.

---

## Flash the Feather as an HA RGB Light

Previously required `--environment feather_rgb_light`. Now use the standard Feather environment with a `neopixel_light` type in the device YAML:

**`devices/feather-rgb-light.yml`** (unchanged — already correct):
```yaml
standalone_modules:
  - type: neopixel_light
    id: rgb_light
    name: RGB Light
    gpio:
      pins: [16]
      roles: [indicator]
```

```sh
pio run --target upload --environment feather_rp2040
```

---

## Flash the Feather as a Status Indicator

Use `neopixel_status` in the YAML — same environment:

```sh
pio run --target upload --environment feather_rp2040
```

---

## Flash the RP2040-Shim as an HA RGB Light (New Capability)

The NeoPixel on the RP2040-Shim (GPIO 23) can now also be configured as an HA RGB light:

```yaml
standalone_modules:
  - type: neopixel_light
    id: rgb_light
    name: RGB Light
    gpio:
      pins: [23]        # onboard NeoPixel on Silicognition RP2040-Shim
      roles: [indicator]
```

```sh
pio run --target upload --environment rp2040_shim
```

---

## Migrating an Existing feather_rgb_light Device

If you previously flashed using `feather_rgb_light`:

1. **No YAML changes needed** — `devices/feather-rgb-light.yml` already uses `type: neopixel_light`
2. **Change the flash command**:
   ```sh
   # Before
   pio run --target upload --environment feather_rgb_light

   # After
   pio run --target upload --environment feather_rp2040
   ```
3. Reflash — the device registers in Home Assistant with the same device ID and topics as before

---

## Verify

```sh
# Run native tests to confirm no regressions
pio test --environment native

# Flash and monitor
pio run --target upload --environment feather_rp2040
pio device monitor
```
