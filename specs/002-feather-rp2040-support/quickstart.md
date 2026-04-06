# Quickstart: Adafruit RP2040 Feather + PoE FeatherWing

This is a board-specific supplement to [001 quickstart](../001-core-framework/quickstart.md). Read that first — this document covers only what differs for the Feather target.

---

## Hardware Required

- Adafruit RP2040 Feather
- Silicognition PoE FeatherWing W5500
- PoE-capable Ethernet switch or PoE injector + Ethernet cable
- USB-C cable (for initial flash only)

Stack the FeatherWing onto the Feather. No soldering required — the headers are press-fit compatible.

**SJIRQ note**: The INT solder jumper on the PoE FeatherWing is default-open. The firmware polls the W5500 rather than using interrupts. Do not bridge SJIRQ — it is not required and will cause pin conflicts with GPIO 12.

---

## 1. Create a Feather Device Config

```sh
cp devices/feather-example.yml devices/my-feather-01.yml
```

Edit `devices/my-feather-01.yml`:

```yaml
device:
  id: my-feather-01
  name: My Feather
  board: adafruit_feather_rp2040

mqtt:
  broker_host: homeassistant.local
  broker_port: 8883
  client_id: my-feather-01
  keepalive_s: 60
  socket_timeout_s: 15
  topic_root: vanlab/my-feather-01

standalone_modules:
  - type: neopixel_status
    id: neopixel_status
    name: Status
    gpio:
      pins: [16]
      roles: [indicator]
```

**Pin 16** is the Feather's onboard NeoPixel. It is not on the header pins — no wiring needed.

**Do not use GPIO 5** — it is not broken out on the Feather. The config validator will reject it at boot.

---

## 2. Build for the Feather

```sh
pio run -e feather_rp2040
```

The `feather_rp2040` environment targets board `adafruit_feather` in the maxgerhardt raspberrypi platform (the YAML `board:` field uses `adafruit_feather_rp2040` for pin validation — this is intentionally different from the PlatformIO board ID). The W5500 is initialized on SPI1 (SCK=18, MOSI=19, MISO=20, CS=10, INT=12) automatically — no YAML configuration required.

---

## 3. Flash

Hold **BOOT** on the Feather, press **RESET**, release BOOT. The Feather mounts as a USB drive. Then:

```sh
pio run -e feather_rp2040 -t upload
```

Or drag the `.uf2` from `.pio/build/feather_rp2040/firmware.uf2` onto the mounted drive.

---

## 4. NeoPixel Status Patterns

The onboard NeoPixel on GPIO 16 shows connection state:

| Color | Pattern | Meaning |
|-------|---------|---------|
| Green | 1 Hz pulse (500ms on/off) | MQTT connected, publishing |
| Yellow | 4 Hz blink (125ms on/off) | Connecting or in backoff |
| Red | Solid on | Config error or fatal condition |

---

## 5. GPIO Pin Reference

Valid user-configurable GPIO pins for `board: adafruit_feather_rp2040`:

```
0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13,
16 (NeoPixel — internal),
18, 19, 20 (SPI1 — shared with W5500),
24, 25, 26, 27, 28, 29
```

**GPIO 5** is absent from the Feather headers. **GPIO 14, 15, 17** are not broken out. Pins 10/12/18/19/20 are used by the W5500 — assigning other modules to these pins will cause hardware conflicts.

---

## 6. Differences from RP2040-Shim

| Item | Feather RP2040 | RP2040-Shim |
|------|---------------|-------------|
| Build env | `feather_rp2040` | `rp2040_shim` |
| W5500 CS | GPIO 10 | GPIO 21 |
| W5500 INT | GPIO 12 | GPIO 14 |
| W5500 SPI | SPI1 (18/19/20) | SPI0 (10/11/12) |
| NeoPixel GPIO | 16 (internal) | 23 (internal) |
| Status LED type | `neopixel_status` | `neopixel_status` |
| Example config | `devices/feather-example.yml` | `devices/example.yml` |

---

## 7. Run Tests

Native tests are board-independent and cover both targets:

```sh
pio test -e native
```

All 62+ tests must pass. `NeoPixelStatusModule` tests use `MockNeoPixelDriver` and run entirely on the host.
