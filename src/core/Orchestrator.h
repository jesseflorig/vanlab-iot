#pragma once
#include "../bundles/base/BundleRegistry.h"
#include "../config/ConfigTypes.h"

class IGPIODriver;
class MQTTClientWrapper;

/**
 * Orchestrator — top-level device coordinator.
 *
 * Owns the BundleRegistry and drives the main setup/loop cycle.
 * Uses MQTTClientWrapper::justReconnected() to trigger HA Discovery
 * publication and MQTT resubscription after every reconnection.
 *
 * Usage:
 *   Orchestrator orch(registry, gpio, mqtt);
 *   orch.setup();
 *   // In Arduino loop():
 *   orch.loop();
 */
class Orchestrator {
public:
    Orchestrator(BundleRegistry& registry,
                 IGPIODriver&    gpio,
                 MQTTClientWrapper& mqtt);

    void setup();
    void loop();

private:
    BundleRegistry&    _registry;
    IGPIODriver&       _gpio;
    MQTTClientWrapper& _mqtt;
};
