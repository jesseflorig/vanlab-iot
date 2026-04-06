#include "DimmerModule.h"
#include "../../../../include/IGPIODriver.h"
#include "../../../mqtt/MQTTClientWrapper.h"
#include <stdio.h>
#include <string.h>

#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef LOW
#define LOW 0
#endif

DimmerModule::DimmerModule(const ModuleConfig& cfg, const DeviceConfig& dev)
    : _id(cfg.id)
    , _pin(cfg.gpio.count > 0 ? (uint8_t)cfg.gpio.pins[0] : 0)
    , _level(0)
    , _stateChanged(false)
{
    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
    snprintf(_stateTopic, sizeof(_stateTopic),
             "%s/%s/state", dev.mqtt.topic_root, cfg.id);
    snprintf(_cmdTopic, sizeof(_cmdTopic),
             "%s/%s/command", dev.mqtt.topic_root, cfg.id);
}

bool DimmerModule::setup(IGPIODriver& gpio) {
    gpio.pinMode(_pin, OUTPUT);
    gpio.analogWrite(_pin, 0);
    return true;
}

void DimmerModule::loop(IGPIODriver& gpio) {
    if (_stateChanged) {
        gpio.analogWrite(_pin, _level);
        _stateChanged = false;
    }
}

bool DimmerModule::publishState(MQTTClientWrapper& mqtt) {
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", _level);
    return mqtt.publish(_stateTopic, payload, true);
}

void DimmerModule::setLevel(uint8_t level) {
    if (_level != level) {
        _level        = level;
        _stateChanged = true;
    }
}

void DimmerModule::toggle() {
    setLevel(_level > 0 ? 0 : 255);
}
