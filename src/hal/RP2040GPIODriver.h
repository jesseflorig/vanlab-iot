#pragma once
#include "../../include/IGPIODriver.h"
#include <Arduino.h>

/**
 * RP2040GPIODriver — thin Arduino-core IGPIODriver implementation.
 *
 * Each method delegates to the corresponding Arduino core function.
 * Used in production firmware; never used in native tests.
 */
class RP2040GPIODriver : public IGPIODriver {
public:
    void     pinMode(uint8_t pin, uint8_t mode) override     { ::pinMode(pin, (PinMode)mode); }
    void     digitalWrite(uint8_t pin, uint8_t val) override { ::digitalWrite(pin, (PinStatus)val); }
    int      digitalRead(uint8_t pin) override               { return ::digitalRead(pin); }
    int      analogRead(uint8_t pin) override                { return ::analogRead(pin); }
    void     analogWrite(uint8_t pin, int val) override      { ::analogWrite(pin, val); }
    uint32_t millis() override                               { return ::millis(); }
    void     delay(uint32_t ms) override                     { ::delay(ms); }
};
