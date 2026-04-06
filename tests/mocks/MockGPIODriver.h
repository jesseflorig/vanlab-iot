#pragma once
#include "../../include/IGPIODriver.h"
#include <stdint.h>

/**
 * MockGPIODriver — IGPIODriver test double.
 *
 * Records all calls for assertion in tests. Provides setters to inject
 * controlled return values for digitalRead/analogRead/millis.
 *
 * Deliberately avoids STL containers to remain portable.
 * Supports up to MAX_PINS GPIO pins and MAX_RECORDED_CALLS recorded calls.
 */
class MockGPIODriver : public IGPIODriver {
public:
    static constexpr uint8_t  MAX_PINS           = 40;
    static constexpr uint16_t MAX_RECORDED_CALLS = 64;

    struct PinModeCall      { uint8_t pin; uint8_t mode; };
    struct DigitalWriteCall { uint8_t pin; uint8_t val; };
    struct AnalogWriteCall  { uint8_t pin; int val; };

    // ── IGPIODriver impl ─────────────────────────────────────────────────────

    void pinMode(uint8_t pin, uint8_t mode) override {
        if (_pinModeCount < MAX_RECORDED_CALLS)
            _pinModeCalls[_pinModeCount++] = {pin, mode};
    }

    void digitalWrite(uint8_t pin, uint8_t val) override {
        if (_digitalWriteCount < MAX_RECORDED_CALLS)
            _digitalWriteCalls[_digitalWriteCount++] = {pin, val};
    }

    int digitalRead(uint8_t pin) override {
        return (pin < MAX_PINS) ? _digitalReadValues[pin] : 0;
    }

    int analogRead(uint8_t pin) override {
        return (pin < MAX_PINS) ? _analogReadValues[pin] : 0;
    }

    void analogWrite(uint8_t pin, int val) override {
        if (_analogWriteCount < MAX_RECORDED_CALLS)
            _analogWriteCalls[_analogWriteCount++] = {pin, val};
    }

    uint32_t millis() override { return _millis; }

    void delay(uint32_t ms) override { _delayCallCount++; }

    // ── Test control ─────────────────────────────────────────────────────────

    void setDigitalRead(uint8_t pin, int value) {
        if (pin < MAX_PINS) _digitalReadValues[pin] = value;
    }
    void setAnalogRead(uint8_t pin, int value) {
        if (pin < MAX_PINS) _analogReadValues[pin] = value;
    }
    void setMillis(uint32_t ms) { _millis = ms; }

    void reset() {
        _pinModeCount      = 0;
        _digitalWriteCount = 0;
        _analogWriteCount  = 0;
        _delayCallCount    = 0;
        _millis            = 0;
        for (int i = 0; i < MAX_PINS; i++) {
            _digitalReadValues[i] = 0;
            _analogReadValues[i]  = 0;
        }
    }

    // ── Call history accessors ───────────────────────────────────────────────

    uint16_t              pinModeCallCount()      const { return _pinModeCount; }
    const PinModeCall&    pinModeCall(int i)       const { return _pinModeCalls[i]; }

    uint16_t              digitalWriteCallCount()  const { return _digitalWriteCount; }
    const DigitalWriteCall& digitalWriteCall(int i) const { return _digitalWriteCalls[i]; }

    uint16_t              analogWriteCallCount()   const { return _analogWriteCount; }
    const AnalogWriteCall& analogWriteCall(int i)  const { return _analogWriteCalls[i]; }

    int                   delayCallCount()         const { return _delayCallCount; }

private:
    PinModeCall       _pinModeCalls[MAX_RECORDED_CALLS]      = {};
    DigitalWriteCall  _digitalWriteCalls[MAX_RECORDED_CALLS] = {};
    AnalogWriteCall   _analogWriteCalls[MAX_RECORDED_CALLS]  = {};

    int _digitalReadValues[MAX_PINS] = {};
    int _analogReadValues[MAX_PINS]  = {};

    uint16_t _pinModeCount      = 0;
    uint16_t _digitalWriteCount = 0;
    uint16_t _analogWriteCount  = 0;
    int      _delayCallCount    = 0;
    uint32_t _millis            = 0;
};
