# Data Model: Feather NeoPixel RGB Light

## New Entities

### NeoPixelLightModule

**File**: `src/bundles/modules/neopixel/NeoPixelLightModule.h/.cpp`
**Role**: `IModule` implementation for HA-controllable RGB light. Subscribes to an MQTT command topic, applies on/off/color/brightness commands to the NeoPixel via `INeoPixelDriver`, and publishes state back to MQTT. Publishes HA Discovery on every MQTT reconnect via `resubscribe()`.

| Field | Type | Description |
|-------|------|-------------|
| _driver | INeoPixelDriver& | Injected NeoPixel driver (hardware or mock) |
| _isOn | bool | Whether the light is currently on |
| _r, _g, _b | uint8_t | Last commanded RGB color (raw, pre-brightness scaling) |
| _brightness | uint8_t | Last commanded brightness (0–255, default 255) |
| _id | const char* | Module ID from ModuleConfig |
| _name | const char* | Module name from ModuleConfig |
| _deviceId | const char* | Device ID from DeviceConfig |
| _deviceName | const char* | Device name from DeviceConfig |
| _commandTopic | char[128] | MQTT command topic: `{topic_root}/light/rgb/set` |
| _stateTopic | char[128] | MQTT state topic: `{topic_root}/light/rgb/state` |
| _discoveryTopic | char[128] | HA Discovery topic: `homeassistant/light/{device_id}/config` |

**Public methods**:

| Method | Signature | Description |
|--------|-----------|-------------|
| Constructor | `NeoPixelLightModule(INeoPixelDriver&, const ModuleConfig&, const DeviceConfig&)` | Initialize fields, build topic strings |
| setup | `void setup(IGPIODriver& gpio, MQTTClientWrapper& mqtt) override` | Call `_driver.begin()`; publish initial state (off) |
| loop | `void loop(IGPIODriver& gpio) override` | No-op (state changes driven by MQTT callback) |
| resubscribe | `void resubscribe(MQTTClientWrapper& mqtt) override` | Subscribe to command topic; publish HA Discovery; publish current state |
| handleCommand | `void handleCommand(const char* payload, unsigned int len)` | Parse JSON, apply state/color/brightness, update NeoPixel, publish state |
| getId | `const char* getId() const override` | Return `_id` |

**State transitions**:

```
OFF → ON (with color):  parse color + brightness → scale → setColor → show → _isOn=true → publishState
OFF → ON (no color):    use last color or default white → _isOn=true → publishState
ON  → OFF:              setColor(0,0,0) → show → _isOn=false → publishState
ON  → color change:     parse color → scale → setColor → show → publishState
brightness-only (OFF):  store brightness only, NeoPixel unchanged
brightness-only (ON):   re-scale current color → setColor → show → publishState
```

**Brightness scaling**: Applied as `scaled = (channel * brightness + 127) / 255` (integer, rounds half-up). Stored R/G/B are raw; driver always receives scaled values.

**Constraints**:
- Module type string: `"neopixel_light"`
- `handleCommand()` must not call any MQTT methods (MQTT-agnostic, testable without MockMQTTClientWrapper)
- NeoPixel retains last-commanded state if MQTT connection drops (no NeoPixel changes on disconnect)
- No flash writes — all state is in-memory only

---

## Updated Entities

### IModule (existing, `include/IModule.h`)

**Change**: Add default no-op `resubscribe()` virtual method.

```cpp
virtual void resubscribe(MQTTClientWrapper& mqtt) {}
```

This is a backward-compatible addition. All existing modules that do not override `resubscribe()` continue to work unchanged.

---

### BundleRegistry (existing, `src/bundles/base/BundleRegistry.h/.cpp`)

**Change**: `resubscribeAll()` now also calls `module.resubscribe(mqtt)` on all registered standalone modules after iterating bundles.

No header signature change — `resubscribeAll(MQTTClientWrapper& mqtt)` parameter already exists.

---

### MQTT Command Schema (inbound from HA)

Topic: `{topic_root}/light/rgb/set`

```json
{
  "state": "ON",
  "color": { "r": 255, "g": 0, "b": 0 },
  "brightness": 255
}
```

All fields optional. Rules:
- `state` absent → no on/off change; color/brightness may still apply
- `color` absent → use last commanded color
- `brightness` absent → use last commanded brightness
- R/G/B values clamped to [0, 255]

---

### MQTT State Schema (published by device)

Topic: `{topic_root}/light/rgb/state`

```json
{
  "state": "ON",
  "color": { "r": 255, "g": 0, "b": 0 },
  "brightness": 255
}
```

Published after every state change and on reconnect. Reflects actual NeoPixel output state.

---

### HA Discovery Payload (published by device)

Topic: `homeassistant/light/{device_id}/config`

```json
{
  "name": "{device_name}",
  "unique_id": "{device_id}_rgb_light",
  "schema": "json",
  "state_topic": "{topic_root}/light/rgb/state",
  "command_topic": "{topic_root}/light/rgb/set",
  "brightness": true,
  "color_mode": true,
  "supported_color_modes": ["rgb"],
  "availability_topic": "{topic_root}/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": ["{device_id}"],
    "name": "{device_name}"
  }
}
```

---

### `devices/feather-rgb-light.yml` (new)

Reference device config for a Feather RGB light device.

```yaml
device:
  id: feather-rgb-light
  name: Feather RGB Light
  board: adafruit_feather_rp2040

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  client_id: feather-rgb-light
  keepalive_s: 60
  socket_timeout_s: 15
  topic_root: vanlab/feather-rgb-light

standalone_modules:
  - type: neopixel_light
    id: rgb_light
    name: RGB Light
    gpio:
      pins: [16]
      roles: [indicator]
```

**Note**: `neopixel_status` is NOT present. The two module types are mutually exclusive in a given device config (GPIO 16 shared).
