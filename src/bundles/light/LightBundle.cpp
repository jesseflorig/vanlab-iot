#include "LightBundle.h"
#include "../../../include/IGPIODriver.h"
#include "../../mqtt/MQTTClientWrapper.h"
#include "../../mqtt/HADiscoveryPayload.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// LightBundle expects cfg.modules[0] = dimmer, cfg.modules[1] = physical_switch.
// This is a fixed use-case bundle; module ordering is part of the contract.

LightBundle::LightBundle(const BundleConfig& cfg, const DeviceConfig& dev)
    : _id(cfg.id)
    , _deviceId(dev.device_id)
    , _deviceName(dev.device_name)
    , _dimmer(cfg.modules[0], dev)
    , _switch(cfg.modules[1], dev)
{
    // Guard against construction with an empty BundleConfig (cfg.type == nullptr).
    // This can occur when a device has no bundles section; in that case the object
    // is never registered and its methods are never called, so a no-op is safe.
    if (!cfg.type || !cfg.id) return;

    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
    snprintf(_stateTopic, sizeof(_stateTopic),
             "%s/%s/state", dev.mqtt.topic_root, cfg.id);
    snprintf(_cmdTopic, sizeof(_cmdTopic),
             "%s/%s/command", dev.mqtt.topic_root, cfg.id);
}

bool LightBundle::setup(IGPIODriver& gpio) {
    // Inject event bus into modules that need it
    _dimmer.setBus(&_bus);
    _switch.setBus(&_bus);

    // Subscribe to switch events before module setups run
    _bus.subscribe(PhysicalSwitchModule::EVENT_SWITCH_PRESSED, onSwitchPressed, this);

    bool ok = _dimmer.setup(gpio);
    ok     &= _switch.setup(gpio);
    return ok;
}

void LightBundle::loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) {
    _dimmer.loop(gpio);
    _switch.loop(gpio);
    _bus.dispatch();
    // State publish happens reactively in handleCommand; MQTT connected state
    // is managed by the Orchestrator which calls publishDiscovery on reconnect.
    (void)mqtt;
}

bool LightBundle::publishDiscovery(MQTTClientWrapper& mqtt) {
    HADiscoveryPayload discovery("light", _deviceId, _id);
    discovery.setName(_id)
             .setStateTopic(_stateTopic)
             .setCommandTopic(_cmdTopic)
             .setAvailabilityTopic(_availTopic)
             .setDeviceInfo(_deviceId, _deviceName, "vanlab-iot");
    return discovery.publish(mqtt);
}

void LightBundle::resubscribe(MQTTClientWrapper& mqtt) {
    mqtt.subscribe(_cmdTopic);
}

void LightBundle::handleCommand(const char* payload, unsigned int len) {
    if (!payload || len == 0) return;
    // Payload is a brightness value 0–255, or "ON"/"OFF"
    if (strncmp(payload, "ON", len) == 0) {
        // Turn on at last non-zero level, or full brightness
        _dimmer.setLevel(_dimmer.getLevel() > 0 ? _dimmer.getLevel() : 255);
    } else if (strncmp(payload, "OFF", len) == 0) {
        _dimmer.setLevel(0);
    } else {
        int level = atoi(payload);
        if (level < 0)   level = 0;
        if (level > 255) level = 255;
        _dimmer.setLevel((uint8_t)level);
    }
}

// static
void LightBundle::onSwitchPressed(const BundleEvent& /*event*/, void* context) {
    LightBundle* self = static_cast<LightBundle*>(context);
    self->_dimmer.toggle();
}
