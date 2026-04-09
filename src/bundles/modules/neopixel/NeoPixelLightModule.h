#pragma once
#include "../../../../include/IModule.h"
#include "../../../../include/INeoPixelDriver.h"
#include "../../../config/ConfigTypes.h"
#include <stdint.h>

class MQTTClientWrapper;

/**
 * NeoPixelLightModule — Home Assistant-controllable RGB light.
 *
 * Drives the onboard NeoPixel as a `light` entity in Home Assistant.
 * Subscribes to an MQTT command topic and applies on/off/color/brightness
 * commands. Publishes state back to MQTT after each change.
 *
 * Command JSON:  {"state":"ON","color":{"r":255,"g":0,"b":0},"brightness":255}
 * State JSON:    {"state":"ON","color":{"r":255,"g":0,"b":0},"brightness":255}
 *
 * Default on first boot: NeoPixel is off. First ON without color → white.
 * Brightness scaling: scaled = (channel * brightness + 127) / 255
 * Module type string: "neopixel_light"
 */
class NeoPixelLightModule : public IModule {
public:
    NeoPixelLightModule(INeoPixelDriver& driver,
                        const ModuleConfig& cfg,
                        const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio)  override;

    bool publishDiscovery(MQTTClientWrapper& mqtt) override;
    bool publishState(MQTTClientWrapper& mqtt)     override;
    void resubscribe(MQTTClientWrapper& mqtt)      override;

    // MQTT-agnostic command handler. Called from main.cpp MQTT callback.
    // Parses JSON payload, updates NeoPixel, stores new state in memory.
    // Does NOT call any MQTT methods — fully testable without MQTTClientWrapper.
    void handleCommand(const char* payload, unsigned int len);

    const char* getId()                const override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

private:
    void applyNeoPixel();

    INeoPixelDriver& _driver;

    bool    _isOn      = false;
    uint8_t _r         = 255;   // last commanded color (raw, pre-brightness)
    uint8_t _g         = 255;
    uint8_t _b         = 255;
    uint8_t _brightness = 255;

    const char* _id;
    const char* _name;
    const char* _deviceId;
    const char* _deviceName;

    char _availTopic[128];
    char _stateTopic[128];
    char _cmdTopic[128];
    char _discoveryTopic[256];
};
