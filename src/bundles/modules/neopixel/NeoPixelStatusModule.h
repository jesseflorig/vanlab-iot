#pragma once
#include "../../../../include/IModule.h"
#include "../../../../include/INeoPixelDriver.h"
#include "../../../config/ConfigTypes.h"

class MQTTClientWrapper;

/**
 * NeoPixelStatusModule — standalone NeoPixel-based status indicator.
 *
 * Drives a single WS2812B NeoPixel with a color pattern that reflects
 * the device's MQTT connection state:
 *
 *   Connected   → green pulse,  1 Hz (500ms on/off)
 *   Connecting  → yellow blink, 4 Hz (125ms on/off)
 *   Error       → red solid (always on, no blink)
 *
 * Never calls millis() directly — uses IGPIODriver::millis() for testability.
 * Module type string: "neopixel_status"
 */
class NeoPixelStatusModule : public IModule {
public:
    enum class StatusState { Connected, Connecting, Error };

    NeoPixelStatusModule(INeoPixelDriver& driver,
                         const ModuleConfig& cfg,
                         const DeviceConfig& dev);

    void setState(StatusState s) { _state = s; }

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio) override;
    bool publishDiscovery(MQTTClientWrapper& mqtt) override;
    bool publishState(MQTTClientWrapper& mqtt) override;

    const char* getId() const                override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

private:
    INeoPixelDriver& _driver;
    StatusState      _state        = StatusState::Connecting;
    uint32_t         _lastToggleMs = 0;
    bool             _lit          = false;
    const char*      _id;
    char             _availTopic[128];
    char             _stateTopic[128];
};
