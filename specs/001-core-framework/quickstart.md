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
  board: silicognition_rp2040_shim

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  client_id: my-device-01
  keepalive_s: 60
  socket_timeout_s: 15
  topic_root: vanlab/my-device-01

standalone_modules:
  - type: status_led
    id: status_led
    name: Status LED
    gpio:
      pins: [25]
      roles: [indicator]
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

Export your broker's CA certificate in PEM format and generate the trust anchor header:

```sh
python scripts/gen_certs.py certs/ca.crt
```

This generates `src/config/trust_anchors.h` containing the CA cert as a PEM string
(`CA_CERT_PEM`). Without this step, TLS verification is disabled at runtime (insecure
mode — for development only).

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

1. Create `src/bundles/modules/<type>/<TypeModule>.h/.cpp` implementing `IModule`
2. Register the module in your bundle's `setup()` or as a standalone in `src/main.cpp`
3. Write unit tests in `tests/test_<type>/test_<type>.cpp`
4. Add the module to a device YAML under `standalone_modules:` or a bundle's `modules:`
5. Rebuild: `pio run -e rp2040_shim`

See `include/IModule.h` for the module interface and `include/IBundle.h` for bundles.
