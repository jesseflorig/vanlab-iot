# Contract: IGPIODriver Interface

## Purpose

`IGPIODriver` abstracts all hardware I/O calls. Module code calls through this
interface — never calling Arduino functions directly. This makes modules fully
testable on the host using `MockGPIODriver` without requiring physical hardware.

## Header

```cpp
// include/IGPIODriver.h

#include <stdint.h>

class IGPIODriver {
public:
    virtual ~IGPIODriver() = default;

    // Pin direction
    virtual void    pinMode(uint8_t pin, uint8_t mode)     = 0;

    // Digital I/O
    virtual void    digitalWrite(uint8_t pin, uint8_t val) = 0;
    virtual int     digitalRead(uint8_t pin)               = 0;

    // Analog I/O
    virtual int     analogRead(uint8_t pin)                = 0;
    virtual void    analogWrite(uint8_t pin, int val)      = 0;

    // Timing (non-blocking time access)
    virtual uint32_t millis()                              = 0;

    // Blocking delay — use ONLY in setup(), never in loop()
    virtual void    delay(uint32_t ms)                     = 0;
};
```

## Implementations

### RP2040GPIODriver (production)

Thin wrapper around Arduino core functions. Lives in `src/hal/RP2040GPIODriver.h`.

```cpp
class RP2040GPIODriver : public IGPIODriver {
public:
    void    pinMode(uint8_t pin, uint8_t mode) override     { ::pinMode(pin, mode); }
    void    digitalWrite(uint8_t pin, uint8_t val) override { ::digitalWrite(pin, val); }
    int     digitalRead(uint8_t pin) override               { return ::digitalRead(pin); }
    int     analogRead(uint8_t pin) override                { return ::analogRead(pin); }
    void    analogWrite(uint8_t pin, int val) override      { ::analogWrite(pin, val); }
    uint32_t millis() override                              { return ::millis(); }
    void    delay(uint32_t ms) override                     { ::delay(ms); }
};
```

### MockGPIODriver (testing)

Records calls and allows test assertions. Lives in `tests/mocks/MockGPIODriver.h`.

```cpp
// Minimal interface — actual implementation will use ArduinoFake or
// a hand-written recorder depending on assertion needs.

class MockGPIODriver : public IGPIODriver {
public:
    // Controllable state
    void    setDigitalRead(uint8_t pin, int value);
    void    setAnalogRead(uint8_t pin, int value);
    void    setMillis(uint32_t ms);

    // Call history for assertions
    struct Call { uint8_t pin; uint8_t val; };
    const std::vector<Call>& digitalWriteCalls() const;
    const std::vector<uint8_t>& pinModeCalls() const;
    int         delayCallCount() const;

    // IGPIODriver impl
    void    pinMode(uint8_t pin, uint8_t mode) override;
    void    digitalWrite(uint8_t pin, uint8_t val) override;
    int     digitalRead(uint8_t pin) override;
    int     analogRead(uint8_t pin) override;
    void    analogWrite(uint8_t pin, int val) override;
    uint32_t millis() override;
    void    delay(uint32_t ms) override;
};
```

## Rules

1. Module code MUST NOT call Arduino functions (`::digitalWrite`, `::millis`, etc.)
   directly. All hardware access goes through the injected `IGPIODriver&`.

2. `delay()` in module code is only acceptable inside `setup()`. Any `delay()` call
   in `loop()` is a constitution violation (blocks the main control loop).

3. Modules receive `IGPIODriver&` as a parameter — they do not store it as a member.

4. The `RP2040GPIODriver` instance is a singleton owned by `main.cpp` and passed by
   reference throughout the call chain.

## Extension

To support I2C, SPI, or UART peripherals, define additional interfaces:

- `II2CDriver` — `beginTransmission`, `write`, `endTransmission`, `requestFrom`, `read`
- `ISPIDriver` — `beginTransaction`, `transfer`, `endTransaction`

Follow the same pattern: abstract base in `include/`, RP2040 impl in `src/hal/`,
mock impl in `tests/mocks/`.
