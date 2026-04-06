#pragma once
#include "../../../../include/IModule.h"
#include "../../../config/ConfigTypes.h"
#include <stdint.h>

class MQTTClientWrapper;
class BundleEventBus;

/**
 * PhysicalSwitchModule — debounced digital input, bundle-owned.
 *
 * Reads a digital pin with 50ms debounce. On a confirmed falling edge
 * (active-low, INPUT_PULLUP assumed), publishes EVENT_SWITCH_PRESSED to
 * the injected BundleEventBus. Bundle owns HA Discovery for this module.
 *
 * Event type constants are defined here so the owning bundle can subscribe
 * using the same identifiers.
 */
class PhysicalSwitchModule : public IModule {
public:
    // Event type IDs published by this module
    static constexpr uint8_t EVENT_SWITCH_PRESSED  = 0;
    static constexpr uint8_t EVENT_SWITCH_RELEASED = 1;

    PhysicalSwitchModule(const ModuleConfig& cfg, const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio) override;
    bool publishDiscovery(MQTTClientWrapper& mqtt) override { return true; } // bundle handles discovery
    bool publishState(MQTTClientWrapper& mqtt) override     { return true; } // no per-module state

    const char* getId() const               override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

    // Called by the owning bundle during its setup() to inject the event bus.
    void setBus(BundleEventBus* bus) override { _bus = bus; }

    bool isPressed() const { return _stableState == 0; }

private:
    static constexpr uint32_t DEBOUNCE_MS = 50;

    const char*    _id;
    uint8_t        _pin;
    BundleEventBus* _bus;

    uint8_t        _stableState;     // last confirmed debounced state
    uint8_t        _pendingState;    // state being timed for debounce
    uint32_t       _debounceStartMs; // when _pendingState was first observed

    char _availTopic[128];
};
