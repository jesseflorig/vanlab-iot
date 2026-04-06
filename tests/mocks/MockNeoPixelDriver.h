#pragma once
#include "../../include/INeoPixelDriver.h"
#include <stdint.h>

/**
 * MockNeoPixelDriver — INeoPixelDriver test double.
 *
 * Records all calls for assertion in native unit tests.
 * No hardware dependency.
 */
class MockNeoPixelDriver : public INeoPixelDriver {
public:
    bool    beginCalled = false;
    uint8_t lastR       = 0;
    uint8_t lastG       = 0;
    uint8_t lastB       = 0;
    uint8_t showCount   = 0;

    void begin() override {
        beginCalled = true;
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b) override {
        lastR = r;
        lastG = g;
        lastB = b;
    }

    void show() override {
        showCount++;
    }

    void reset() {
        beginCalled = false;
        lastR       = 0;
        lastG       = 0;
        lastB       = 0;
        showCount   = 0;
    }
};
