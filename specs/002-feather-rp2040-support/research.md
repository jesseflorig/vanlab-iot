# Research: Adafruit RP2040 Feather Board Support

## Decision 1: NeoPixel GPIO Pins

**Decision**: Shim NeoPixel = GPIO 23; Feather NeoPixel = GPIO 16 (internal, not on Feather headers).

**Rationale**: Confirmed from earlephilhower arduino-pico core variant files (`variants/silicognition_rp2040_shim/pins_arduino.h`: `PIN_NEOPIXEL = 23u`) and CircuitPython board definitions. Neither board has a NeoPixel power enable pin — the NeoPixels are always powered from 3.3V.

**Alternatives considered**: GPIO 17 (incorrectly assumed to be NeoPixel power on Feather) — confirmed non-existent; GPIO 25 (current status LED on Shim) — plain LED, not NeoPixel.

---

## Decision 2: PoE FeatherWing W5500 SPI Pin Assignments

**Decision**: W5500 pins are board-specific due to the Shim's custom GPIO-to-pad remapping:

| Signal | Feather RP2040 | RP2040-Shim |
|--------|---------------|-------------|
| CS (D10) | GPIO 10 | GPIO 21 |
| INT (D12, SJIRQ) | GPIO 12 | GPIO 14 |
| RST | RC circuit | RC circuit |
| SCK | GPIO 18 (SPI1) | GPIO 10 (SPI0) |
| MOSI | GPIO 19 (SPI1) | GPIO 11 (SPI0) |
| MISO | GPIO 20 (SPI1) | GPIO 12 (SPI0) |

**Rationale**: The PoE FeatherWing is physically Feather-pin-compatible but the RP2040-Shim remaps Feather header positions to different GPIO numbers (e.g., Feather D10 pad → GPIO 21 on Shim vs. GPIO 10 on Feather RP2040). Confirmed from Silicognition product documentation and Hackaday.io project page.

**INT jumper**: SJIRQ is default-open on the PoE FeatherWing. The W5500 will be polled (software loop) rather than interrupt-driven. This is acceptable — PubSubClient and ESP_SSLClient are loop-based.

**RST**: No host GPIO is connected to W5500 RST on either board. Software reset uses the W5500's internal reset register. No RST pin tracking needed in firmware.

**Alternatives considered**: Interrupt-driven Ethernet (SJIRQ bridged) — deferred; adds complexity without user-visible benefit at this stage.

---

## Decision 3: W5500 Ethernet Initialization Strategy

**Decision**: Board-specific W5500 init in `main.cpp`, driven by compile-time macros set via `build_flags` in `platformio.ini`.

```ini
[env:feather_rp2040]
build_flags = ... -DBOARD_FEATHER_RP2040

[env:rp2040_shim]
build_flags = ... -DBOARD_RP2040_SHIM
```

```cpp
// main.cpp
#ifdef BOARD_FEATHER_RP2040
  static Wiznet5500lwIP eth(/*CS=*/10, SPI1, /*INT=*/12);
#elif defined(BOARD_RP2040_SHIM)
  static Wiznet5500lwIP eth(/*CS=*/21, SPI0, /*INT=*/14);
#endif
```

**Rationale**: The SPI bus and pin assignments for the W5500 are fixed hardware properties of each board combination — not user-configurable. Build flags are the correct mechanism for compile-time hardware constants. This resolves an inherited gap from 001: the W5500 was never initialized in the current firmware.

**Alternatives considered**:
- YAML-configurable Ethernet pins — rejected; Ethernet SPI wiring is fixed by the board, not the application.
- Runtime board detection — rejected; RP2040 has no runtime board ID; adds complexity with no benefit.

---

## Decision 4: NeoPixel Library

**Decision**: `adafruit/Adafruit NeoPixel` (PlatformIO lib_deps identifier).

**Rationale**: Mature library with explicit RP2040 support since v1.10.2 via the earlephilhower PIO backend (clock-independent, bit-perfect timing). Straightforward API (`setPixelColor`, `show`). Well-tested with PubSubClient in existing RP2040 projects. Single-pixel use case has negligible memory footprint.

**Alternatives considered**:
- `Adafruit_NeoPXL8` (DMA-based) — overkill for a single status LED.
- Direct PIO assembly — too low-level; no benefit over the library for this use case.
- PIO-based encoder libraries (potential conflict) — not in use in this project; no conflict.

---

## Decision 5: NeoPixel Driver Abstraction

**Decision**: Introduce `INeoPixelDriver` interface (pure virtual, C++14) alongside the existing `IGPIODriver`. Concrete implementation `RP2040NeoPixelDriver` wraps `Adafruit_NeoPixel` and is `#ifdef ARDUINO` gated. `MockNeoPixelDriver` in `tests/mocks/` records `setColor` and `show` calls for unit testing.

**Rationale**: Constitution Principle V requires mockable hardware interfaces. `Adafruit_NeoPixel` is Arduino-only and cannot link on native (host) builds. `INeoPixelDriver` follows the exact same pattern as `IGPIODriver` and requires zero new patterns for implementors to learn.

```cpp
class INeoPixelDriver {
public:
    virtual ~INeoPixelDriver() = default;
    virtual void begin() = 0;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void show() = 0;
};
```

**Alternatives considered**:
- Add NeoPixel methods to `IGPIODriver` — rejected; bleeds WS2812B specifics into the general GPIO abstraction.
- Stub out `Adafruit_NeoPixel` for native — rejected; more fragile than a clean interface.

---

## Decision 6: Feather RP2040 Valid GPIO Pin Set

**Decision**: Valid user-configurable GPIO pins for `adafruit_feather_rp2040` board validation:
```
{0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 16, 18, 19, 20, 24, 25, 26, 27, 28, 29}
```

GPIO 16 (NeoPixel) is included despite being internal-only on the Feather headers because it's the designated NeoPixel pin and must be configurable in device YAML.

**Rationale**: The Feather RP2040 breaks out 22 GPIOs on its headers. GPIO 5 is absent (not broken out). GPIO 14–17 are not on headers except 16 (NeoPixel) which is the target of this feature. Confirmed from Adafruit's Feather RP2040 pinout documentation and CircuitPython pins.c.

**Alternatives considered**: Allowing all 30 RP2040 GPIOs — rejected; pins not on the headers cannot be wired to external peripherals, so permitting them would produce silent hardware failures.

---

## Decision 7: NeoPixelStatusModule Blink Pattern

**Decision**: Three-color status pattern, consistent across both boards:
- **Green pulse** (slow, 1Hz): MQTT connected and publishing
- **Yellow blink** (fast, 4Hz): Connecting / backoff
- **Red solid**: Config error or fatal condition

**Rationale**: Color-coded status is more informative than the existing monochrome StatusLEDModule blink pattern. The existing module remains available for plain LED hardware. Three states cover the primary observable conditions from the MQTT FSM (Connected, Backoff/Connecting, Error).

**Alternatives considered**: Single-color brightness modulation — less readable. HA Discovery publishing color status — out of scope for this feature.
