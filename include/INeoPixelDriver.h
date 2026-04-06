#pragma once
#include <stdint.h>

/**
 * INeoPixelDriver — pure virtual interface for a single-pixel WS2812B NeoPixel.
 *
 * Analogous to IGPIODriver — all NeoPixel hardware access goes through this
 * interface. Implemented by RP2040NeoPixelDriver (hardware) and
 * MockNeoPixelDriver (native tests).
 *
 * Usage:
 *   1. Call begin() once in setup().
 *   2. Call setColor(r, g, b) to stage a color.
 *   3. Call show() to transmit the staged color to the pixel.
 */
class INeoPixelDriver {
public:
    virtual ~INeoPixelDriver() = default;

    // Initialize the NeoPixel hardware. Safe to call more than once.
    virtual void begin() = 0;

    // Stage an RGB color value. Must call show() to transmit.
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;

    // Transmit the last staged color to the NeoPixel.
    virtual void show() = 0;
};
