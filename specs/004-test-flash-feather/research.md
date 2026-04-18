# Research: Validate Local Testing and Flashing to RP2040 Feather via USB-C

**Feature**: 004-test-flash-feather  
**Date**: 2026-04-18

## Findings

### Native Test Execution

**Decision**: Use `pio test --environment native`  
**Rationale**: PlatformIO's `native` environment is already configured in `platformio.ini` with `build_src_filter` that excludes hardware-dependent files (`main.cpp`, `MQTTClientWrapper.cpp`). All 10 test suites in `tests/` are auto-discovered by PlatformIO's Unity-based runner — no registration required.  
**Alternatives considered**: Running tests with a standalone test framework (e.g., CMake + GoogleTest) — rejected because PlatformIO already provides test discovery, and splitting the toolchain would add friction without benefit.

### Flash to Feather

**Decision**: `pio run --target upload --environment feather_rp2040`  
**Rationale**: `feather_rp2040` is the base Feather environment. `feather_rgb_light` is a variant with `NEOPIXEL_LIGHT_MODE` enabled — same board, different firmware role. Developers should be directed to choose the environment matching their device config.  
**Alternatives considered**: Using `pio run --target upload` without `--environment` — rejected because `default_envs = rp2040_shim` means omitting the flag targets the wrong board.

### README Bug: Wrong Environment Name

**Finding**: The README Getting Started section instructs users to run `pio run --target upload --environment rp2040`. The environment `rp2040` does not exist in `platformio.ini`. The correct environment names for flash targets are `rp2040_shim`, `feather_rp2040`, and `feather_rgb_light`.  
**Impact**: A developer following the README for a Feather board will get a PlatformIO error. This is a documentation defect.  
**Fix**: Update the Getting Started section to use the correct Feather environment names and add the native test command.

### Serial Monitor

**Decision**: `pio device monitor` (no flags needed for Feather)  
**Rationale**: PlatformIO auto-detects the connected USB serial device. The `monitor_speed = 115200` setting in all Feather environments is picked up automatically. On macOS the Feather typically enumerates as `/dev/cu.usbmodem*`.  
**Alternatives considered**: Specifying `--port` explicitly — unnecessary for single-device workflows; PlatformIO handles auto-detection.

### CLAUDE.md Commands Gap

**Finding**: The `Commands` section in `CLAUDE.md` reads `# Add commands for C++14, Arduino framework` — no commands are documented. This is a known gap (auto-generated placeholder).  
**Fix**: Populate the Commands section with the canonical PlatformIO commands for this project.

### No Contracts or Data Model

**Finding**: This feature adds no new code paths, modules, or interfaces. It is a workflow validation and documentation task. A `contracts/` directory and `data-model.md` are not applicable.
