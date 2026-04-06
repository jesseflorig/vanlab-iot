# Quickstart: vanlab-iot Core Framework

## Prerequisites

- [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/installation/index.html)
- Python 3.8+ (for build scripts)
- `pip install pyyaml` (for YAML→C++ codegen)
- A running MQTT broker with TLS (Mosquitto recommended)
- Home Assistant with MQTT integration enabled

## 1. Create a Device Config

Copy the example and edit for your device:

```sh
cp devices/example.yml devices/my-device-01.yml
```

Edit `devices/my-device-01.yml`:

```yaml
device:
  id: my-device-01
  name: My Device
  board: silicognition-rp2040-shim

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  keepalive: 60
  topic_root: vanlab/my-device-01

modules:
  - type: status_led
    id: status
    name: Status LED
    gpio:
      pin: 25
```

## 2. Provision Runtime Secrets

Create `/config/runtime.json` on a blank LittleFS partition (or upload via serial
after first flash — see Step 5):

```json
{
  "mqtt_username": "your_mqtt_user",
  "mqtt_password": "your_mqtt_password",
  "ca_cert_path": "/certs/ca.crt"
}
```

## 3. Provision TLS Certificate

Export your broker's CA certificate in PEM format. Convert it to a BearSSL trust
anchor header:

```sh
python scripts/gen_certs.py certs/ca.crt > src/config/trust_anchors.h
```

This generates a `const br_x509_trust_anchor TAs[]` array compiled into firmware.

## 4. Build

Set the target device in `platformio.ini` (or via `--environment`):

```sh
pio run --environment rp2040_shim
```

The build process automatically:
1. Runs `scripts/gen_config.py` on your device YAML → `src/config/generated_config.h`
2. Compiles firmware with the generated config baked in

## 5. Flash

Hold the BOOTSEL button on the RP2040-Shim while connecting USB, then:

```sh
pio run --target upload --environment rp2040_shim
```

First boot only — upload the LittleFS partition with runtime secrets and certs:

```sh
pio run --target uploadfs --environment rp2040_shim
```

Ensure `/data/config/runtime.json` and `/data/certs/ca.crt` exist in the project
root before running `uploadfs`.

## 6. Verify

Monitor serial output:

```sh
pio device monitor --baud 115200
```

Expected boot sequence:

```
[BOOT] vanlab-iot v0.1.0
[CONFIG] Loaded: my-device-01 (silicognition-rp2040-shim)
[CONFIG] Modules: 1 (status_led)
[RUNTIME] Loaded runtime config from LittleFS
[ETH] Ethernet initializing...
[ETH] Connected. IP: 192.168.1.42
[MQTT] Connecting to homeassistant.local:8883...
[MQTT] Connected.
[MQTT] Publishing discovery for: status
[MQTT] Discovery published. Device online.
```

Check Home Assistant → Settings → Devices → search "My Device".

## 7. Run Tests

Host-side unit tests (no hardware needed):

```sh
pio test --environment native
```

## Adding a Module

1. Create `src/modules/<type>/<TypeModule>.h` implementing `IModule`
2. Register the factory in `src/modules/ModuleRegistry.cpp`
3. Add the `type` string to `contracts/device-config-schema.md`
4. Write unit tests in `tests/unit/modules/test_<type>.cpp`
5. Add the module to a device YAML config and rebuild

See `contracts/module-interface.md` for the full module contract.
