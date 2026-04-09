# Contract: NeoPixel Light MQTT Interface

## Overview

The `NeoPixelLightModule` exposes three MQTT topics forming its external interface with Home Assistant. All topics are under the device's configured `topic_root`.

---

## Topic 1: Command (Subscribe)

**Topic**: `{topic_root}/light/rgb/set`
**Direction**: Device subscribes (HA publishes)
**QoS**: 0
**Retained**: No

### Payload Schema

```json
{
  "state": "ON | OFF",
  "color": { "r": 0-255, "g": 0-255, "b": 0-255 },
  "brightness": 0-255
}
```

All fields are optional. The device applies only the fields present:

| Field | Type | Required | Behavior if absent |
|-------|------|----------|--------------------|
| state | string | No | No on/off change |
| color | object | No | Use last commanded color |
| brightness | integer | No | Use last commanded brightness |

**Validation rules**:
- R/G/B values outside [0, 255] are clamped — command is not rejected
- Unknown JSON fields are ignored
- Non-JSON or unparseable payloads are silently dropped

### Example Payloads

Turn on with red at half brightness:
```json
{"state": "ON", "color": {"r": 255, "g": 0, "b": 0}, "brightness": 128}
```

Turn off:
```json
{"state": "OFF"}
```

Change color while on:
```json
{"color": {"r": 0, "g": 0, "b": 255}}
```

Pre-set brightness while off:
```json
{"brightness": 200}
```

---

## Topic 2: State (Publish)

**Topic**: `{topic_root}/light/rgb/state`
**Direction**: Device publishes (HA subscribes)
**QoS**: 0
**Retained**: Yes

### Payload Schema

```json
{
  "state": "ON | OFF",
  "color": { "r": 0-255, "g": 0-255, "b": 0-255 },
  "brightness": 0-255
}
```

All fields always present. Published after every state change and on every MQTT reconnect.

### Example

```json
{"state": "ON", "color": {"r": 255, "g": 0, "b": 0}, "brightness": 128}
```

---

## Topic 3: HA Discovery (Publish)

**Topic**: `homeassistant/light/{device_id}/config`
**Direction**: Device publishes (HA subscribes)
**QoS**: 0
**Retained**: Yes

Published once on every MQTT connection. Causes HA to auto-register the light entity.

### Required Fields

| Field | Value |
|-------|-------|
| schema | `"json"` |
| state_topic | `{topic_root}/light/rgb/state` |
| command_topic | `{topic_root}/light/rgb/set` |
| brightness | `true` |
| color_mode | `true` |
| supported_color_modes | `["rgb"]` |
| availability_topic | `{topic_root}/status` |

---

## Availability Integration

The device publishes availability via the existing framework mechanism:
- `{topic_root}/status` → `"online"` on connect (LWT: `"offline"`)

The discovery payload references this topic so HA marks the entity unavailable on device disconnect.
