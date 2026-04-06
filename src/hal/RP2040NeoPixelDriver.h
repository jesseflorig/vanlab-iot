#pragma once

#ifdef ARDUINO

#include <Adafruit_NeoPixel.h>
#include "../../include/INeoPixelDriver.h"

/**
 * RP2040NeoPixelDriver — concrete INeoPixelDriver for RP2040 boards.
 *
 * Wraps Adafruit_NeoPixel for a single-pixel WS2812B NeoPixel.
 * Compiled only for Arduino targets (#ifdef ARDUINO).
 *
 * Usage:
 *   RP2040NeoPixelDriver driver(16); // GPIO 16 = Feather onboard NeoPixel
 *   driver.begin();
 *   driver.setColor(0, 50, 0);  // stage green
 *   driver.show();              // transmit
 */
class RP2040NeoPixelDriver : public INeoPixelDriver {
public:
    explicit RP2040NeoPixelDriver(uint8_t pin)
        : _pin(pin)
        , _np(1, pin, NEO_GRB + NEO_KHZ800)
    {}

    void begin() override {
        _np.begin();
        _np.clear();
        _np.show();
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b) override {
        _np.setPixelColor(0, r, g, b);
    }

    void show() override {
        _np.show();
    }

private:
    uint8_t           _pin;
    Adafruit_NeoPixel _np;
};

#endif // ARDUINO
