#pragma once
#include "../../../../include/IModule.h"
#include "../../../config/ConfigTypes.h"

class MQTTClientWrapper;

/**
 * StatusLEDModule — standalone indicator LED.
 *
 * Blinks the configured LED pin at a fixed interval using millis() guards.
 * Publishes its state as a standalone HA binary_sensor entity.
 * Never calls delay() — fully non-blocking.
 */
class StatusLEDModule : public IModule {
public:
    StatusLEDModule(const ModuleConfig& cfg, const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio) override;
    bool publishDiscovery(MQTTClientWrapper& mqtt) override;
    bool publishState(MQTTClientWrapper& mqtt) override;

    const char* getId() const               override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

    bool isOn() const { return _state; }

private:
    const char* _id;
    uint8_t     _pin;
    bool        _state;
    uint32_t    _lastToggleMs;
    char        _availTopic[128];
    char        _stateTopic[128];

    static constexpr uint32_t BLINK_INTERVAL_MS = 500;
};
