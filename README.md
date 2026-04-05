# vanlab-iot

A framework for building secure, modular MQTT clients on PoE RP2040 devices. Devices are
fully configurable via YAML, integrate with Home Assistant out of the box via MQTT
Discovery, and operate reliably offline-first over encrypted MQTTS.

## Target Hardware

| Component | Part |
|-----------|------|
| MCU | Raspberry Pi RP2040 |
| Board | Silicognition RP2040-Shim |
| Ethernet | Silicognition FeatherWing PoE (W5500) |
| Connectivity | Ethernet over PoE — no Wi-Fi dependency |

## Key Features

- **Offline-first** — devices buffer state and reconnect with backoff; the control loop
  never blocks on network I/O
- **MQTTS (TLS)** — all broker communication is encrypted; plaintext MQTT is forbidden
- **Home Assistant Discovery** — every module publishes HA-compatible discovery payloads
  automatically on startup and reconnect
- **YAML-configurable** — GPIO assignments, MQTT topics, and module composition are
  defined in per-device YAML config files; nothing is hardcoded in firmware
- **GPIO-aware** — pin assignments are validated against the target board's pinout at
  config load time
- **Modular architecture** — capabilities (sensors, relays, status LEDs, etc.) are
  composed as independent, testable modules

## Toolchain

- **Framework**: Arduino (via PlatformIO)
- **Language**: C++14
- **Build**: PlatformIO Core

## Project Structure

```
devices/          # Per-device YAML configuration files
src/              # Framework source
  modules/        # Pluggable capability modules
  hal/            # Hardware abstraction layer (mockable for testing)
  config/         # YAML config loader and schema validation
  mqtt/           # MQTTS client, reconnect logic, discovery payloads
  core/           # Main loop and module orchestration
tests/
  unit/           # Host-side unit tests (no hardware required)
  integration/    # Full MQTTS cycle tests
lib/              # Vendored or managed dependencies
platformio.ini    # PlatformIO project config
```

## Getting Started

1. Install [PlatformIO](https://platformio.org/install/cli)
2. Copy `devices/example.yml` to `devices/<your-device>.yml` and configure GPIO and MQTT
3. Provision a TLS certificate for the device and reference it in the device config
4. Build and flash:
   ```sh
   pio run --target upload --environment rp2040
   ```
5. Monitor serial output:
   ```sh
   pio device monitor
   ```

## Configuration

Each device is described by a YAML file in `devices/`. Example:

```yaml
device:
  id: workshop-sensor-01
  board: silicognition-rp2040-shim

mqtt:
  broker: mqtts://homeassistant.local:8883
  credentials_file: secrets/broker.yml

modules:
  - type: temperature
    gpio: 4
    topic: workshop/temperature
  - type: relay
    gpio: 16
    topic: workshop/relay/1
```

## Constitution

Project principles and governance are defined in [`.specify/memory/constitution.md`](.specify/memory/constitution.md).
