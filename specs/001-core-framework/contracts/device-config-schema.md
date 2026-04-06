# Contract: Device Config YAML Schema

## Purpose

Every device is described by a YAML file in `devices/`. This file is the single source
of truth for device identity, MQTT configuration, and module composition. It is
converted to `src/config/generated_config.h` at build time by `scripts/gen_config.py`.

## Schema Version: 1

## Full Schema

```yaml
# devices/<device-id>.yml
# Schema version: 1

device:
  id: <string>           # REQUIRED. Unique identifier. Used as MQTT client ID,
                         # HA device registry key, and LittleFS namespace.
                         # Pattern: ^[a-z0-9][a-z0-9_-]{0,62}$
  name: <string>         # REQUIRED. Human-readable name for HA device registry.
  board: <string>        # REQUIRED. Board identifier. Currently supported:
                         #   silicognition-rp2040-shim

mqtt:
  broker_host: <string>  # REQUIRED. Hostname or IP. No scheme prefix.
  broker_port: <int>     # Default: 8883
  keepalive: <int>       # Default: 60 (seconds)
  socket_timeout: <int>  # Default: 15 (seconds)
  topic_root: <string>   # Default: "vanlab/<device.id>"
  # Credentials and TLS paths come from runtime.json on LittleFS, not here.

# Bundles — named use-case profiles; each owns one or more modules and an event bus.
# Bundle behaviors are hardcoded per bundle type; params customize values only.
bundles:
  - type: <string>       # REQUIRED. Registered bundle type, e.g. "light", "environment"
    id: <string>         # REQUIRED. Unique within device.
                         # Pattern: ^[a-z0-9][a-z0-9_-]{0,30}$
    name: <string>       # REQUIRED. Human-readable; used as prefix for HA entity names.
    topic_prefix: <string>  # Default: "<mqtt.topic_root>/<bundle.id>"

    params:              # Bundle-type-specific values (optional). Customizes hardcoded
      <key>: <value>     # behavior without changing logic. E.g. night_dim_level: 30

    modules:             # REQUIRED. Must satisfy the bundle type's required module set.
      - type: <string>   # Module type identifier
        id: <string>     # Unique within bundle
        gpio:
          <role>: <int>  # GPIO pin assignments validated at build time
        params:
          <key>: <value>

# Standalone modules — registered directly with the orchestrator; no bundle coordination.
# Use for infrastructure concerns: status LEDs, watchdogs, diagnostic publishers.
modules:
  - type: <string>
    id: <string>         # Unique within device (across bundles and standalone)
    name: <string>
    topic_prefix: <string>
    poll_interval_ms: <int>
    gpio:
      <role>: <int>
    params:
      <key>: <value>
```

## Supported Bundle Types (v1)

| `type` | Required Modules | Description |
|--------|-----------------|-------------|
| `light` | `dimmer`, `physical_switch`, `night_mode` | Dimmable light with offline switch and night mode |
| `environment` | `dht22_temperature` | Temperature + humidity monitoring |

Additional bundle types are added by implementing `IBundle` and registering a factory
in `BundleRegistry`.

## Supported Module Types (v1)

| `type` | Required GPIO Roles | Optional GPIO Roles | Description |
|--------|--------------------|--------------------|-------------|
| `dht22_temperature` | `data` | — | DHT22 temperature + humidity sensor |
| `dimmer` | `control` | `feedback` | PWM dimmer output |
| `physical_switch` | `input` | — | Offline-capable physical input |
| `night_mode` | — | — | Schedule-based mode flag |
| `relay` | `control` | `feedback` | Single relay output |
| `binary_sensor` | `input` | — | Digital input / door/window sensor |
| `status_led` | `pin` | — | Single LED indicator |
| `analog_sensor` | `input` | — | Generic analog input (ADC) |

Additional module types are added by implementing `IModule` and registering a factory.

## Runtime Config (LittleFS: `/config/runtime.json`)

Secrets and per-deployment values that must not be compiled into firmware:

```json
{
  "mqtt_username": "device_user",
  "mqtt_password": "s3cr3t",
  "ca_cert_path": "/certs/ca.crt",
  "client_cert_path": "/certs/client.crt",
  "client_key_path": "/certs/client.key"
}
```

`client_cert_path` and `client_key_path` are optional (only needed for mTLS).

## Validated Constraints

The codegen script enforces at build time:

- `device.id` matches pattern `^[a-z0-9][a-z0-9_-]{0,62}$`
- All `id` fields unique within their scope (bundle IDs unique per device;
  module IDs unique within their bundle; standalone module IDs unique per device)
- Each bundle's `modules` list satisfies the bundle type's required module set
- All GPIO pins legal for the declared `device.board`
- No two modules within a device share the same GPIO pin
- `poll_interval_ms` is 0 or ≥ 100
- Bundle count ≤ `MAX_BUNDLES` (default: 8)
- Total module count ≤ `MAX_MODULES` (default: 16, across all bundles + standalone)

## Example Device Config

```yaml
# devices/workshop-node-01.yml

device:
  id: workshop-node-01
  name: Workshop Node
  board: silicognition-rp2040-shim

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  topic_root: vanlab/workshop-node-01

bundles:
  - type: light
    id: overhead
    name: Overhead Light
    params:
      night_dim_level: 25
    modules:
      - type: dimmer
        id: dimmer
        gpio:
          control: 16
      - type: physical_switch
        id: switch
        gpio:
          input: 17
      - type: night_mode
        id: night
        params:
          start: "22:00"
          end: "06:00"

  - type: environment
    id: env
    name: Workshop Environment
    modules:
      - type: dht22_temperature
        id: temp
        gpio:
          data: 4
        params:
          units: celsius
          poll_interval_ms: 30000

modules:
  - type: status_led
    id: status
    name: Status LED
    gpio:
      pin: 25
```

In HA this device appears as **Workshop Node** with entities:
- Overhead Light (light entity — from `light` bundle)
- Workshop Temperature (sensor — from `environment` bundle)
- Workshop Humidity (sensor — from `environment` bundle)
- Status LED (standalone — if configured for HA Discovery)
