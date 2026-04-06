# Contract: INeoPixelDriver Interface

**Feature**: 002-feather-rp2040-support
**File**: `include/INeoPixelDriver.h`
**Pattern**: Follows `IGPIODriver` — pure virtual interface, injected via reference, mockable in native tests.

---

## Interface Definition

```cpp
class INeoPixelDriver {
public:
    virtual ~INeoPixelDriver() = default;
    virtual void begin() = 0;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void show() = 0;
};
```

---

## Method Contracts

### `begin()`

| Item | Value |
|------|-------|
| Precondition | None — safe to call at any time, intended for `setup()` |
| Postcondition | Hardware initialized; `setColor` and `show` may be called |
| Side effects | Hardware implementation: initializes Adafruit_NeoPixel (`begin()`), clears pixel. Mock: sets `beginCalled = true` |
| Idempotency | MUST be safe to call more than once |

### `setColor(uint8_t r, uint8_t g, uint8_t b)`

| Item | Value |
|------|-------|
| Precondition | `begin()` has been called |
| Postcondition | Color staged; NOT yet transmitted to hardware |
| Side effects | Hardware implementation: calls `Adafruit_NeoPixel::setPixelColor(0, r, g, b)`. Mock: records `lastR`, `lastG`, `lastB` |
| Notes | Must call `show()` after to transmit the staged color |

### `show()`

| Item | Value |
|------|-------|
| Precondition | `begin()` has been called |
| Postcondition | Last staged color transmitted to NeoPixel hardware |
| Side effects | Hardware implementation: calls `Adafruit_NeoPixel::show()`. Mock: increments `showCount` |
| Timing | Hardware: WS2812B protocol requires ~30µs per pixel; blocks for that duration |

---

## Concrete Implementations

### RP2040NeoPixelDriver (hardware)

- **File**: `src/hal/RP2040NeoPixelDriver.h`
- **Guard**: `#ifdef ARDUINO` — excluded from native builds
- **Constructor**: `RP2040NeoPixelDriver(uint8_t pin)` — stores pin, constructs single-pixel `Adafruit_NeoPixel(1, pin, NEO_GRB + NEO_KHZ800)`
- **Library**: `adafruit/Adafruit NeoPixel` >= 1.10.2

### MockNeoPixelDriver (testing)

- **File**: `tests/mocks/MockNeoPixelDriver.h`
- **Purpose**: INeoPixelDriver test double for native unit tests; no hardware required
- **Public fields** (readable in assertions):

| Field | Type | Initial | Set by |
|-------|------|---------|--------|
| `beginCalled` | `bool` | `false` | `begin()` |
| `lastR` | `uint8_t` | `0` | `setColor()` |
| `lastG` | `uint8_t` | `0` | `setColor()` |
| `lastB` | `uint8_t` | `0` | `setColor()` |
| `showCount` | `uint8_t` | `0` | `show()` |

- **Reset method**: `void reset()` — resets all fields to initial values (useful between test cases)

---

## Usage by NeoPixelStatusModule

`NeoPixelStatusModule` takes `INeoPixelDriver&` in its constructor. It MUST NOT call hardware directly. All pixel operations go through the driver interface.

```cpp
// Construction
RP2040NeoPixelDriver driver(16); // hardware, pin 16
NeoPixelStatusModule statusModule(driver);

// Or for tests:
MockNeoPixelDriver mockDriver;
NeoPixelStatusModule statusModule(mockDriver);
```

---

## Test Requirements

Unit tests in `tests/test_neopixel_module/test_neopixel_module.cpp` MUST cover:

| Test | Assertion |
|------|-----------|
| `setup()` calls `driver.begin()` | `mockDriver.beginCalled == true` after `module.setup()` |
| Connected state shows green | After `setState(Connected)` + `loop()` tick, `lastR==0, lastG>0, lastB==0` |
| Connecting state shows yellow | After `setState(Connecting)` + `loop()` tick, `lastR>0, lastG>0, lastB==0` |
| Error state shows red solid | After `setState(Error)` + `loop()` tick, `lastR>0, lastG==0, lastB==0` |
| Blink timer toggles correctly | `showCount` increments on each phase change at correct intervals |
| Error state does not blink | `showCount` does not increase beyond 1 over multiple `loop()` ticks in Error state |
