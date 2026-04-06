# Contract: MQTT Topic Conventions

## Topic Root

```
vanlab/<device_id>/
```

All device topics are under this root unless `mqtt.topic_root` is overridden in the
device config.

## Standard Topics

### Availability

```
vanlab/<device_id>/availability
```

Payload: `online` | `offline`  
Retained: yes  
Published by: framework on connect (`online`) and via MQTT LWT (`offline`)

### Module State

```
vanlab/<device_id>/<module_id>/state
```

Payload: JSON object or scalar — format defined per module type  
Retained: yes  
Published by: the module on state change, and by the framework after reconnection

### Module Command (controllable modules only)

```
vanlab/<device_id>/<module_id>/command
```

Payload: command string (e.g. `ON`, `OFF`, `TOGGLE`) or JSON  
Retained: no  
Subscribed by: the module after each MQTT connection

### Module Availability (per-module)

```
vanlab/<device_id>/<module_id>/availability
```

Payload: `online` | `offline`  
Retained: yes  
Published by: the module in `setup()` (`online`) and via LWT (`offline`)

---

## HA Discovery Topics

```
homeassistant/<component>/<device_id>/<object_id>/config
```

Where:
- `<component>` is the HA entity type: `sensor`, `switch`, `binary_sensor`, `light`, etc.
- `<device_id>` is `DeviceConfig.device_id`
- `<object_id>` is `ModuleConfig.id`

Payload: JSON (see `contracts/ha-discovery.md`)  
Retained: yes  
Published by: each module's `publishDiscovery()` on every MQTT connection

---

## Payload Formats by Module Type

### dht22_temperature — state topic

```json
{
  "temperature": 22.5,
  "humidity": 48.2,
  "unit": "C"
}
```

### relay — state topic

```
ON
```
or
```
OFF
```

### relay — command topic

```
ON | OFF | TOGGLE
```

### binary_sensor — state topic

```
ON
```
or
```
OFF
```

### analog_sensor — state topic

```json
{
  "value": 512,
  "voltage": 1.65
}
```

---

## Rules

1. All state topics MUST use `retain: true`.
2. Discovery topics MUST use `retain: true`.
3. Command topics MUST NOT use `retain`.
4. Topic strings MUST use lowercase with hyphens. No spaces, no uppercase.
5. Module IDs used in topics must match `ModuleConfig.id` exactly.
6. All devices MUST publish to the device-level `availability` topic with LWT set
   to `offline`.
