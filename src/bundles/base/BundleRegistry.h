#pragma once
#include "../../../include/IBundle.h"
#include "../../../include/IModule.h"
#include <stdint.h>

class IGPIODriver;
class MQTTClientWrapper;

/**
 * BundleRegistry — orchestrates all bundles and standalone modules.
 *
 * Called by the Orchestrator each loop tick. Manages both IBundle instances
 * (which own their modules) and standalone IModule instances.
 */
class BundleRegistry {
public:
    static constexpr uint8_t MAX_BUNDLES = 8;
    static constexpr uint8_t MAX_STANDALONE = 8;

    BundleRegistry();

    void registerBundle(IBundle* bundle);
    void registerModule(IModule* module);  // standalone modules only

    void setup(IGPIODriver& gpio);
    void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt);
    void publishAllDiscovery(MQTTClientWrapper& mqtt);
    void resubscribeAll(MQTTClientWrapper& mqtt);

    uint8_t bundleCount()     const { return _bundleCount; }
    uint8_t standaloneCount() const { return _standaloneCount; }

private:
    IBundle* _bundles[MAX_BUNDLES];
    IModule* _standalone[MAX_STANDALONE];
    uint8_t  _bundleCount;
    uint8_t  _standaloneCount;
};
