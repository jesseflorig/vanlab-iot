#include "Orchestrator.h"
#include "../mqtt/MQTTClientWrapper.h"
#include "../../include/IGPIODriver.h"

Orchestrator::Orchestrator(BundleRegistry&    registry,
                           IGPIODriver&       gpio,
                           MQTTClientWrapper& mqtt)
    : _registry(registry)
    , _gpio(gpio)
    , _mqtt(mqtt)
{
}

void Orchestrator::setup() {
    _mqtt.begin();
    _registry.setup(_gpio);
}

void Orchestrator::loop() {
    _mqtt.loop();

    if (_mqtt.justReconnected()) {
        // Publish HA Discovery for all bundles and standalone modules,
        // then re-subscribe to all command topics.
        _registry.publishAllDiscovery(_mqtt);
        _registry.resubscribeAll(_mqtt);
    }

    // Run registry every tick — hardware state (PWM, LED blink) must be maintained
    // even while offline.
    _registry.loop(_gpio, _mqtt);
}
