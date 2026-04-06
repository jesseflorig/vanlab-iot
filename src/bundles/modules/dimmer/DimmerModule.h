#pragma once
#include "../../../../include/IModule.h"
#include "../../../config/ConfigTypes.h"
#include <stdint.h>

class MQTTClientWrapper;

/**
 * DimmerModule — PWM dimmer output, bundle-owned.
 *
 * Controls a PWM output pin. Receives commands via MQTT command topic or
 * directly via setLevel()/toggle() called by the owning bundle in response
 * to BundleEvents. Bundle owns HA Discovery for this module.
 */
class DimmerModule : public IModule {
public:
    DimmerModule(const ModuleConfig& cfg, const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio) override;
    bool publishDiscovery(MQTTClientWrapper& mqtt) override { return true; } // bundle handles discovery
    bool publishState(MQTTClientWrapper& mqtt) override;

    const char* getId() const               override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

    // Called by owning bundle in response to events or MQTT commands
    void setLevel(uint8_t level);
    void toggle();
    uint8_t getLevel() const { return _level; }

private:
    const char* _id;
    uint8_t     _pin;
    uint8_t     _level;
    bool        _stateChanged;
    char        _availTopic[128];
    char        _stateTopic[128];
    char        _cmdTopic[128];
};
