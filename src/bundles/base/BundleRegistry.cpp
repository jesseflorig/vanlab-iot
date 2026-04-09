#include "BundleRegistry.h"
#include "../../../include/IGPIODriver.h"
#include "../../mqtt/MQTTClientWrapper.h"

BundleRegistry::BundleRegistry()
    : _bundleCount(0), _standaloneCount(0) {}

void BundleRegistry::registerBundle(IBundle* bundle) {
    if (_bundleCount < MAX_BUNDLES)
        _bundles[_bundleCount++] = bundle;
}

void BundleRegistry::registerModule(IModule* module) {
    if (_standaloneCount < MAX_STANDALONE)
        _standalone[_standaloneCount++] = module;
}

void BundleRegistry::setup(IGPIODriver& gpio) {
    for (int i = 0; i < _bundleCount;     i++) _bundles[i]->setup(gpio);
    for (int i = 0; i < _standaloneCount; i++) _standalone[i]->setup(gpio);
}

void BundleRegistry::loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) {
    for (int i = 0; i < _bundleCount;     i++) _bundles[i]->loop(gpio, mqtt);
    for (int i = 0; i < _standaloneCount; i++) _standalone[i]->loop(gpio);
}

void BundleRegistry::publishAllDiscovery(MQTTClientWrapper& mqtt) {
    for (int i = 0; i < _bundleCount;     i++) _bundles[i]->publishDiscovery(mqtt);
    for (int i = 0; i < _standaloneCount; i++) _standalone[i]->publishDiscovery(mqtt);
}

void BundleRegistry::resubscribeAll(MQTTClientWrapper& mqtt) {
    for (int i = 0; i < _bundleCount;     i++) _bundles[i]->resubscribe(mqtt);
    for (int i = 0; i < _standaloneCount; i++) _standalone[i]->resubscribe(mqtt);
}
