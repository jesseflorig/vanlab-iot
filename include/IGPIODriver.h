#pragma once
#include <stdint.h>

/**
 * IGPIODriver — hardware abstraction for all GPIO operations.
 *
 * All module code calls through this interface, never invoking Arduino core
 * functions directly. This makes modules fully testable on the host using
 * MockGPIODriver without requiring physical hardware.
 *
 * See contracts/hal-interface.md for the full contract.
 */
class IGPIODriver {
public:
    virtual ~IGPIODriver() = default;

    virtual void     pinMode(uint8_t pin, uint8_t mode)     = 0;
    virtual void     digitalWrite(uint8_t pin, uint8_t val) = 0;
    virtual int      digitalRead(uint8_t pin)               = 0;
    virtual int      analogRead(uint8_t pin)                = 0;
    virtual void     analogWrite(uint8_t pin, int val)      = 0;

    // Timing — use millis() for non-blocking guards; delay() only in setup()
    virtual uint32_t millis()                               = 0;
    virtual void     delay(uint32_t ms)                     = 0;
};
