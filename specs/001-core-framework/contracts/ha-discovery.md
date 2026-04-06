# Contract: Home Assistant MQTT Discovery

## Overview

Every module MUST publish a valid HA MQTT Discovery payload on each MQTT connection.
This allows Home Assistant to automatically register and configure entities without
manual configuration.

Reference: https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery

## Discovery Topic

```
homeassistant/<component>/<device_id>/<object_id>/config
```

- `<component>`: HA entity type (sensor, switch, binary_sensor, light, etc.)
- `<device_id>`: `DeviceConfig.device_id`
- `<object_id>`: `ModuleConfig.id`

Payload: JSON, `retain: true`

## Required Payload Fields

Every discovery payload MUST include:

| Field | Value | Notes |
|-------|-------|-------|
| `unique_id` | `<device_id>_<module_id>` | Stable across reboots |
| `name` | `ModuleConfig.name` | Human-readable entity name |
| `state_topic` | `vanlab/<device_id>/<module_id>/state` | |
| `availability_topic` | `vanlab/<device_id>/availability` | Device-level availability |
| `payload_available` | `online` | |
| `payload_not_available` | `offline` | |
| `device` | (see Device Object below) | Groups entities in HA UI |

## Device Object (required in every payload)

```json
{
  "identifiers": ["<device_id>"],
  "name": "<DeviceConfig.device_name>",
  "model": "<DeviceConfig.board>",
  "manufacturer": "vanlab-iot"
}
```

## Component-Specific Fields

### sensor

```json
{
  "unique_id": "workshop-env-01_temp",
  "name": "Workshop Temperature",
  "state_topic": "vanlab/workshop-env-01/temp/state",
  "value_template": "{{ value_json.temperature }}",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "state_class": "measurement",
  "availability_topic": "vanlab/workshop-env-01/availability",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": { ... }
}
```

### switch

```json
{
  "unique_id": "workshop-env-01_fan",
  "name": "Workshop Fan",
  "state_topic": "vanlab/workshop-env-01/fan/state",
  "command_topic": "vanlab/workshop-env-01/fan/command",
  "payload_on": "ON",
  "payload_off": "OFF",
  "state_on": "ON",
  "state_off": "OFF",
  "availability_topic": "vanlab/workshop-env-01/availability",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": { ... }
}
```

### binary_sensor

```json
{
  "unique_id": "workshop-env-01_door",
  "name": "Workshop Door",
  "state_topic": "vanlab/workshop-env-01/door/state",
  "payload_on": "ON",
  "payload_off": "OFF",
  "device_class": "door",
  "availability_topic": "vanlab/workshop-env-01/availability",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": { ... }
}
```

## Deletion / Removal

To remove an entity from HA, publish an empty payload (`""`) to the discovery topic
with `retain: true`. The framework does not do this automatically — it is a manual
decommission step.

## Rules

1. `retain: true` is MANDATORY for all discovery payloads.
2. `unique_id` MUST be stable across reboots and firmware updates.
3. `device` object MUST be present in every payload to ensure HA groups entities.
4. Payloads MUST be valid JSON. Validate in `publishDiscovery()` before publishing.
5. `HADiscoveryPayload::isValid()` MUST return `true` before calling
   `mqtt.publish()`. Log an error and skip if invalid.
