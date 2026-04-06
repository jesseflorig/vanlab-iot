#include "NeoPixelStatusModule.h"
#include "../../../../include/IGPIODriver.h"
#include "../../../mqtt/MQTTClientWrapper.h"
#include "../../../mqtt/HADiscoveryPayload.h"
#include <stdio.h>
#include <string.h>

// Blink intervals
static constexpr uint32_t CONNECTED_INTERVAL_MS  = 500;  // 1 Hz
static constexpr uint32_t CONNECTING_INTERVAL_MS = 125;  // 4 Hz

// Status colors (low brightness to avoid blinding)
static constexpr uint8_t COLOR_CONNECTED_R  = 0;
static constexpr uint8_t COLOR_CONNECTED_G  = 50;
static constexpr uint8_t COLOR_CONNECTED_B  = 0;

static constexpr uint8_t COLOR_CONNECTING_R = 50;
static constexpr uint8_t COLOR_CONNECTING_G = 50;
static constexpr uint8_t COLOR_CONNECTING_B = 0;

static constexpr uint8_t COLOR_ERROR_R = 50;
static constexpr uint8_t COLOR_ERROR_G = 0;
static constexpr uint8_t COLOR_ERROR_B = 0;

NeoPixelStatusModule::NeoPixelStatusModule(INeoPixelDriver& driver,
                                           const ModuleConfig& cfg,
                                           const DeviceConfig& dev)
    : _driver(driver)
    , _id(cfg.id)
{
    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
    snprintf(_stateTopic, sizeof(_stateTopic),
             "%s/%s/state", dev.mqtt.topic_root, cfg.id);
}

bool NeoPixelStatusModule::setup(IGPIODriver& /*gpio*/) {
    _driver.begin();
    return true;
}

void NeoPixelStatusModule::loop(IGPIODriver& gpio) {
    uint32_t now = gpio.millis();

    if (_state == StatusState::Error) {
        // Solid red — only update once when state enters Error
        if (!_lit) {
            _driver.setColor(COLOR_ERROR_R, COLOR_ERROR_G, COLOR_ERROR_B);
            _driver.show();
            _lit = true;
        }
        return;
    }

    uint32_t interval = (_state == StatusState::Connected)
                        ? CONNECTED_INTERVAL_MS
                        : CONNECTING_INTERVAL_MS;

    if (now - _lastToggleMs >= interval) {
        _lit = !_lit;
        if (_lit) {
            if (_state == StatusState::Connected) {
                _driver.setColor(COLOR_CONNECTED_R, COLOR_CONNECTED_G, COLOR_CONNECTED_B);
            } else {
                _driver.setColor(COLOR_CONNECTING_R, COLOR_CONNECTING_G, COLOR_CONNECTING_B);
            }
        } else {
            _driver.setColor(0, 0, 0);
        }
        _driver.show();
        _lastToggleMs = now;
    }
}

bool NeoPixelStatusModule::publishDiscovery(MQTTClientWrapper& mqtt) {
    HADiscoveryPayload discovery("binary_sensor", _id, _id);
    discovery.setName("NeoPixel Status")
             .setStateTopic(_stateTopic)
             .setAvailabilityTopic(_availTopic);
    return discovery.publish(mqtt);
}

bool NeoPixelStatusModule::publishState(MQTTClientWrapper& mqtt) {
    const char* state = (_state == StatusState::Connected) ? "ON" : "OFF";
    return mqtt.publish(_stateTopic, state, true);
}
