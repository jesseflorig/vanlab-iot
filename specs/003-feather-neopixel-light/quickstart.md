# Quickstart: Feather RGB Light

This is a device-config supplement. Read [001 quickstart](../001-core-framework/quickstart.md) and [002 quickstart](../002-feather-rp2040-support/quickstart.md) first — this document covers only what differs for the `feather-rgb-light` configuration.

---

## What This Config Does

The `feather-rgb-light` config registers the RP2040 Feather's onboard NeoPixel as a Home Assistant `light` entity. From HA, you can turn it on/off, change its RGB color, and adjust brightness. The device publishes its current state back to MQTT so HA always reflects the actual NeoPixel output.

**Note**: This config uses `neopixel_light` on GPIO 16. Do not combine it with `neopixel_status` (feature 002) — they share the same GPIO pin. Use one or the other in a given device config.

---

## 1. Create the Device Config

```sh
cp devices/feather-rgb-light.yml devices/my-rgb-light.yml
```

Edit `devices/my-rgb-light.yml`:

```yaml
device:
  id: my-rgb-light
  name: My RGB Light
  board: adafruit_feather_rp2040

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  client_id: my-rgb-light
  keepalive_s: 60
  socket_timeout_s: 15
  topic_root: vanlab/my-rgb-light

standalone_modules:
  - type: neopixel_light
    id: rgb_light
    name: RGB Light
    gpio:
      pins: [16]
      roles: [indicator]
```

Fill in your MQTT broker address and credentials (via runtime secrets — see 001 quickstart).

---

## 2. Build

```sh
pio run -e feather_rp2040
```

The `DEVICE_CONFIG_FILE` build flag must point to your device YAML. If you created `devices/my-rgb-light.yml`, update `platformio.ini` or pass it via `build_flags`:

```ini
-DDEVICE_CONFIG_FILE='"devices/my-rgb-light.yml"'
```

---

## 3. Flash

Hold **BOOT**, press **RESET**, release BOOT. Then:

```sh
pio run -e feather_rp2040 -t upload
```

---

## 4. Verify in Home Assistant

After the device boots and connects to MQTT:

1. In HA → Settings → Devices & Services → MQTT → Entities — a new `light` entity should appear automatically (no manual configuration).
2. The entity name matches the `name` field in your device YAML.
3. Open a light card and send an ON command with red (R=255, G=0, B=0). The NeoPixel on the Feather should change to red within one second.
4. Send OFF. The NeoPixel turns off. HA reflects the OFF state.

---

## 5. MQTT Topics

For a device with `topic_root: vanlab/my-rgb-light` and `id: my-rgb-light`:

| Purpose | Topic |
|---------|-------|
| Command (from HA) | `vanlab/my-rgb-light/light/rgb/set` |
| State (from device) | `vanlab/my-rgb-light/light/rgb/state` |
| HA Discovery | `homeassistant/light/my-rgb-light/config` |
| Availability | `vanlab/my-rgb-light/status` |

You can test manually with an MQTT client:

```sh
# Send ON with green
mosquitto_pub -h homeassistant.local -t "vanlab/my-rgb-light/light/rgb/set" \
  -m '{"state":"ON","color":{"r":0,"g":255,"b":0},"brightness":255}'

# Send OFF
mosquitto_pub -h homeassistant.local -t "vanlab/my-rgb-light/light/rgb/set" \
  -m '{"state":"OFF"}'
```

---

## 6. Reconnect Behavior

If the device loses MQTT connectivity:
- The NeoPixel retains its last-commanded state (does not turn off)
- On reconnect, the device republishes HA Discovery and its current state
- HA reflects the current state within a few seconds of reconnect

---

## 7. Run Tests

```sh
pio test -e native
```

All existing tests plus new `test_neopixel_light` tests must pass. The light module tests use `MockNeoPixelDriver` and run entirely on the host — no hardware required.
