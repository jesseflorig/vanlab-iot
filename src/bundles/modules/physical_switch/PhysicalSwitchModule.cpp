#include "PhysicalSwitchModule.h"
#include "../../../../include/IGPIODriver.h"
#include "../../../bundles/base/BundleEventBus.h"
#include <stdio.h>
#include <string.h>

#ifndef INPUT_PULLUP
#define INPUT_PULLUP 2
#endif
#ifndef HIGH
#define HIGH 1
#define LOW  0
#endif

PhysicalSwitchModule::PhysicalSwitchModule(const ModuleConfig& cfg, const DeviceConfig& dev)
    : _id(cfg.id)
    , _pin(cfg.gpio.count > 0 ? (uint8_t)cfg.gpio.pins[0] : 0)
    , _bus(nullptr)
    , _stableState(HIGH)   // pulled up; not pressed
    , _pendingState(HIGH)
    , _debounceStartMs(0)
{
    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
}

bool PhysicalSwitchModule::setup(IGPIODriver& gpio) {
    gpio.pinMode(_pin, INPUT_PULLUP);
    _stableState    = (uint8_t)gpio.digitalRead(_pin);
    _pendingState   = _stableState;
    _debounceStartMs = 0;
    return true;
}

void PhysicalSwitchModule::loop(IGPIODriver& gpio) {
    uint32_t now = gpio.millis();
    uint8_t  raw = (uint8_t)gpio.digitalRead(_pin);

    if (raw != _pendingState) {
        // State changed — reset debounce timer
        _pendingState    = raw;
        _debounceStartMs = now;
    } else if ((now - _debounceStartMs) >= DEBOUNCE_MS && _pendingState != _stableState) {
        // Stable for DEBOUNCE_MS and different from last confirmed state
        uint8_t prev   = _stableState;
        _stableState   = _pendingState;

        if (_bus) {
            if (_stableState == LOW && prev == HIGH) {
                // Falling edge → pressed
                _bus->publish({EVENT_SWITCH_PRESSED, 0, nullptr});
            } else if (_stableState == HIGH && prev == LOW) {
                // Rising edge → released
                _bus->publish({EVENT_SWITCH_RELEASED, 0, nullptr});
            }
        }
    }
}
