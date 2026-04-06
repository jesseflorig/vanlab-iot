#include "StatusLEDModule.h"
#include "../../../../include/IGPIODriver.h"
#include "../../../mqtt/MQTTClientWrapper.h"
#include "../../../mqtt/HADiscoveryPayload.h"
#include <stdio.h>
#include <string.h>

// Arduino pin constants not available on native
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef HIGH
#define HIGH 1
#define LOW  0
#endif

StatusLEDModule::StatusLEDModule(const ModuleConfig& cfg, const DeviceConfig& dev)
    : _id(cfg.id)
    , _pin(cfg.gpio.count > 0 ? (uint8_t)cfg.gpio.pins[0] : 25)
    , _state(false)
    , _lastToggleMs(0)
{
    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
    snprintf(_stateTopic, sizeof(_stateTopic),
             "%s/%s/state", dev.mqtt.topic_root, cfg.id);
}

bool StatusLEDModule::setup(IGPIODriver& gpio) {
    gpio.pinMode(_pin, OUTPUT);
    gpio.digitalWrite(_pin, LOW);
    return true;
}

void StatusLEDModule::loop(IGPIODriver& gpio) {
    uint32_t now = gpio.millis();
    if (now - _lastToggleMs >= BLINK_INTERVAL_MS) {
        _state = !_state;
        gpio.digitalWrite(_pin, _state ? HIGH : LOW);
        _lastToggleMs = now;
    }
}

bool StatusLEDModule::publishDiscovery(MQTTClientWrapper& mqtt) {
    // Extract device info from availability topic prefix
    // Note: device_id and device_name come from DeviceConfig, stored at construction
    HADiscoveryPayload discovery("binary_sensor", _id, _id);
    discovery.setName("Status LED")
             .setStateTopic(_stateTopic)
             .setAvailabilityTopic(_availTopic);
    // Device info stored separately; full impl would pass DeviceConfig to this method
    return discovery.publish(mqtt);
}

bool StatusLEDModule::publishState(MQTTClientWrapper& mqtt) {
    return mqtt.publish(_stateTopic, _state ? "ON" : "OFF", true);
}
