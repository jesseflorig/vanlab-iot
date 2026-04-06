# Contract: Board Build Environments

**Feature**: 002-feather-rp2040-support
**File**: `platformio.ini`

Defines how a `board:` value in device YAML maps to a PlatformIO build environment, and what each environment enforces at compile time and boot time.

---

## Board → Environment Mapping

| YAML `board:` value | PlatformIO env | Build macro | NeoPixel GPIO | W5500 CS | W5500 INT | SPI bus |
|--------------------|----------------|-------------|---------------|----------|-----------|---------|
| `adafruit_feather_rp2040` | `feather_rp2040` | `BOARD_FEATHER_RP2040` | 16 | GPIO 10 | GPIO 12 | SPI1 |
| `silicognition_rp2040_shim` | `rp2040_shim` | `BOARD_RP2040_SHIM` | 23 | GPIO 21 | GPIO 14 | SPI0 |

The `board:` field in device YAML is informational — it drives config validation (pin set enforcement), but the build environment is selected at compile time via `pio run -e <env>`. Mismatching them (e.g., building `rp2040_shim` env but loading a config with `board: adafruit_feather_rp2040`) will produce a pin validation error at boot.

---

## `[env:feather_rp2040]`

```ini
[env:feather_rp2040]
platform       = https://github.com/maxgerhardt/platform-raspberrypi.git
board          = adafruit_feather
board_build.core             = earlephilhower
board_build.filesystem_size  = 1m
lib_deps       =
    knolleary/PubSubClient
    mobizt/ESP_SSLClient
    bblanchon/ArduinoJson
    adafruit/Adafruit NeoPixel
build_flags    =
    -DBOARD_FEATHER_RP2040
    -DDEVICE_CONFIG_FILE='"devices/feather-example.yml"'
extra_scripts  = pre:scripts/gen_config.py
```

---

## `[env:rp2040_shim]` (updated)

```ini
[env:rp2040_shim]
; ... existing keys unchanged ...
lib_deps       =
    knolleary/PubSubClient
    mobizt/ESP_SSLClient
    bblanchon/ArduinoJson
    adafruit/Adafruit NeoPixel          ; ← added
build_flags    =
    -DBOARD_RP2040_SHIM                 ; ← added
    -DDEVICE_CONFIG_FILE='"devices/example.yml"'
```

Only `lib_deps` (adding NeoPixel) and `build_flags` (adding `BOARD_RP2040_SHIM`) change from the 001 baseline.

---

## W5500 Initialization Contract

Board-specific `Wiznet5500lwIP` initialization lives in `main.cpp`, gated by build macros. No user-configurable YAML is involved — these are fixed hardware properties.

```cpp
#ifdef BOARD_FEATHER_RP2040
  static Wiznet5500lwIP eth(/*CS=*/10, SPI1, /*INT=*/12);
  void setupSPI() { SPI1.setRX(20); SPI1.setTX(19); SPI1.setSCK(18); }
#elif defined(BOARD_RP2040_SHIM)
  static Wiznet5500lwIP eth(/*CS=*/21, SPI0, /*INT=*/14);
  void setupSPI() { SPI0.setRX(12); SPI0.setTX(11); SPI0.setSCK(10); }
#else
  #error "No board target defined. Use -DBOARD_FEATHER_RP2040 or -DBOARD_RP2040_SHIM"
#endif
```

`setupSPI()` must be called before `eth.begin()` in `setup()` to configure the SPI pins for the earlephilhower core.

---

## GPIO Pin Set Enforcement

`ConfigLoader` validates every pin in `GpioConfig.pins[]` against the board's compile-time `BOARD_VALID_PINS` array. On mismatch, it logs an error and halts.

### Feather RP2040 Valid Pins

```cpp
#ifdef BOARD_FEATHER_RP2040
static const uint8_t BOARD_VALID_PINS[] = {
    0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13,
    16,           // NeoPixel (internal, not on headers)
    18, 19, 20,   // SPI1 (also W5500 SCK/MOSI/MISO)
    24, 25,
    26, 27, 28, 29
};
static const uint8_t BOARD_VALID_PINS_COUNT = 23;
#endif
```

GPIO 5 is absent (not broken out on Feather). GPIO 14, 15, 17 are not on headers and not in the set. GPIO 16 is included because it is the NeoPixel pin.

### RP2040-Shim Valid Pins

Pin set for `BOARD_RP2040_SHIM` uses the existing validation logic from 001 (full range with Shim-specific exclusions). No change required.

---

## Validation Rules

| Rule | Enforcement point | Failure behavior |
|------|------------------|-----------------|
| Pin in `BOARD_VALID_PINS` | `ConfigLoader::validate()` at boot | Log error, call `while(1)` halt |
| `board:` field matches build env | Runtime warning only (cannot enforce at compile time) | Log warning, continue |
| `#error` if no board macro defined | Compile time | Build fails with message |
