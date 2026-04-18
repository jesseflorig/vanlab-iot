# Quickstart: Local Testing and Flashing to RP2040 Feather

**Feature**: 004-test-flash-feather  
**Date**: 2026-04-18

## Prerequisites

- [PlatformIO Core CLI](https://platformio.org/install/cli) installed and on your PATH
- For flashing: Adafruit RP2040 Feather connected via a USB-C data cable
- For flashing: A valid device YAML config file in `devices/` (e.g., `devices/feather-example.yml` or `devices/feather-rgb-light.yml`)

---

## Run Native Unit Tests (No Hardware Required)

```sh
pio test --environment native
```

All test suites in `tests/` are discovered and run automatically. Hardware-dependent files are excluded by the `native` environment's `build_src_filter`. Expected output ends with a pass/fail count per suite.

---

## Flash Firmware to Feather

Choose the environment that matches your device config:

| Environment       | When to use                                                   |
|-------------------|---------------------------------------------------------------|
| `feather_rp2040`  | Standard Feather — no NeoPixel light mode                     |
| `feather_rgb_light` | Feather with NeoPixel as an HA-controllable RGB light       |

```sh
# Standard Feather
pio run --target upload --environment feather_rp2040

# Feather with NeoPixel RGB light
pio run --target upload --environment feather_rgb_light
```

The pre-build script (`scripts/gen_config.py`) runs automatically and generates `src/generated_config.h` from the device YAML before compilation.

---

## Monitor Serial Output

```sh
pio device monitor
```

PlatformIO auto-detects the connected Feather's serial port. Baud rate is 115200 (set in `platformio.ini`). Run this after flashing to observe startup logs, MQTT connection attempts, and module initialization.

---

## Workflow Summary

```
1. Edit firmware or config
2. pio test --environment native     ← confirm logic; no hardware needed
3. pio run --target upload --environment feather_rp2040   ← flash
4. pio device monitor                ← observe serial output
```
