# Data Model: Adafruit RP2040 Feather Board Support

## New Entities

### INeoPixelDriver

**File**: `include/INeoPixelDriver.h`
**Role**: Pure virtual interface abstracting a single-pixel WS2812B NeoPixel. Analogous to `IGPIODriver` — all NeoPixel hardware access goes through this interface.

| Method | Signature | Description |
|--------|-----------|-------------|
| begin | `virtual void begin() = 0` | Initialize the NeoPixel hardware (pixel count, pin, type) |
| setColor | `virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0` | Stage an RGB color value |
| show | `virtual void show() = 0` | Transmit staged color to the NeoPixel |

**Relationships**: Implemented by `RP2040NeoPixelDriver` (hardware) and `MockNeoPixelDriver` (testing).

---

### RP2040NeoPixelDriver

**File**: `src/hal/RP2040NeoPixelDriver.h`
**Role**: Concrete `INeoPixelDriver` wrapping `Adafruit_NeoPixel`. Compiled only for Arduino targets (`#ifdef ARDUINO`). Holds pin number and single-pixel `Adafruit_NeoPixel` instance.

| Field | Type | Description |
|-------|------|-------------|
| _pin | uint8_t | NeoPixel data GPIO pin |
| _np | Adafruit_NeoPixel | Single-pixel NeoPixel instance (TYPE_GRB + KHZ800) |

**Constraints**: Single pixel only (`n=1`). Pin must be within the board's valid NeoPixel pin.

---

### MockNeoPixelDriver

**File**: `tests/mocks/MockNeoPixelDriver.h`
**Role**: `INeoPixelDriver` test double. Records all calls for assertion in unit tests.

| Field | Type | Description |
|-------|------|-------------|
| lastR, lastG, lastB | uint8_t | Last color passed to `setColor` |
| showCount | uint8_t | Number of times `show()` was called |
| beginCalled | bool | Whether `begin()` was called |

---

### NeoPixelStatusModule

**File**: `src/bundles/modules/neopixel/NeoPixelStatusModule.h/.cpp`
**Role**: `IModule` implementation for NeoPixel-based status indication. Holds an `INeoPixelDriver&` and drives blink patterns based on an internal state.

| Field | Type | Description |
|-------|------|-------------|
| _driver | INeoPixelDriver& | Injected NeoPixel driver |
| _state | StatusState | Current blink state (Connected, Connecting, Error) |
| _lastToggleMs | uint32_t | Last blink toggle timestamp |
| _lit | bool | Current NeoPixel on/off phase within blink cycle |

**State transitions**:

```
StatusState enum:
  Connected   → green pulse, 1 Hz (500ms on, 500ms off)
  Connecting  → yellow blink, 4 Hz (125ms on, 125ms off)
  Error       → red solid (always on)
```

**Constraints**:
- Module type string: `"neopixel_status"`
- `setup()` calls `_driver.begin()`
- `loop()` drives blink timer via `gpio.millis()` (no direct `millis()` call — uses injected GPIO driver for testability)
- `setState(StatusState)` is the public control API called by Orchestrator or main.cpp

---

### BoardPinSet (compile-time, not a C++ class)

**Role**: The set of valid GPIO pin numbers for a given build environment, enforced during device config validation at boot.

| Board | Valid GPIO pins |
|-------|----------------|
| adafruit_feather_rp2040 | 0,1,2,3,4,6,7,8,9,10,11,12,13,16,18,19,20,24,25,26,27,28,29 |
| silicognition_rp2040_shim | Full RP2040 GPIO range (per existing validation) |

**Validation rule**: If `GpioConfig.pins[i]` is not in the board's valid set, ConfigLoader must log an error and the device must halt.

**Implementation**: `BOARD_VALID_PINS` compile-time constant array set via `build_flags`.

---

## Updated Entities

### DeviceConfig (existing, `src/config/ConfigTypes.h`)

No struct changes. The `board` field already exists (`const char* board`). Validation logic in `ConfigLoader` is extended to use the board-specific pin set.

---

### `devices/example.yml` (existing, updated)

The `standalone_modules` entry changes from `status_led` on GPIO 25 to `neopixel_status` on GPIO 23:

```yaml
# Before
standalone_modules:
  - type: status_led
    gpio: {pins: [25], roles: [indicator]}

# After
standalone_modules:
  - type: neopixel_status
    gpio: {pins: [23], roles: [indicator]}
```

---

## New Build Environment Entity

### `[env:feather_rp2040]` (`platformio.ini`)

| Key | Value |
|-----|-------|
| platform | `https://github.com/maxgerhardt/platform-raspberrypi.git` |
| board | `adafruit_feather` |
| board_build.core | `earlephilhower` |
| board_build.filesystem_size | `1m` |
| lib_deps | PubSubClient + ESP_SSLClient + ArduinoJson + **Adafruit NeoPixel** |
| build_flags | `-DBOARD_FEATHER_RP2040 -DDEVICE_CONFIG_FILE='"devices/feather-example.yml"'` |
| extra_scripts | `pre:scripts/gen_config.py` |

**Shared**: `[env:rp2040_shim]` gains `-DBOARD_RP2040_SHIM` build flag and `adafruit/Adafruit NeoPixel` lib dep.
