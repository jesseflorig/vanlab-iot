#include "NeoPixelLightModule.h"
#include "../../../../include/IGPIODriver.h"
#include "../../../mqtt/MQTTClientWrapper.h"
#include <stdio.h>
#include <string.h>

#include <ArduinoJson.h>

NeoPixelLightModule::NeoPixelLightModule(INeoPixelDriver& driver,
                                         const ModuleConfig& cfg,
                                         const DeviceConfig& dev)
    : _driver(driver)
    , _id(cfg.id)
    , _name(cfg.name)
    , _deviceId(dev.device_id)
    , _deviceName(dev.device_name)
{
    snprintf(_availTopic, sizeof(_availTopic),
             "%s/availability", dev.mqtt.topic_root);
    snprintf(_stateTopic, sizeof(_stateTopic),
             "%s/%s/state", dev.mqtt.topic_root, cfg.id);
    snprintf(_cmdTopic, sizeof(_cmdTopic),
             "%s/%s/set", dev.mqtt.topic_root, cfg.id);
    snprintf(_discoveryTopic, sizeof(_discoveryTopic),
             "homeassistant/light/%s/%s/config", dev.device_id, cfg.id);
}

bool NeoPixelLightModule::setup(IGPIODriver& /*gpio*/) {
    _driver.begin();
    return true;
}

void NeoPixelLightModule::loop(IGPIODriver& /*gpio*/) {
    // State changes are driven entirely by MQTT commands via handleCommand().
    // Nothing to do on each tick.
}

bool NeoPixelLightModule::publishDiscovery(MQTTClientWrapper& mqtt) {
    // Build RGB light discovery payload manually — HADiscoveryPayload does not
    // support the schema/brightness/color_mode fields required by HA for RGB lights.
    char payload[1024];
    snprintf(payload, sizeof(payload),
        "{"
        "\"unique_id\":\"%s_%s\","
        "\"name\":\"%s\","
        "\"schema\":\"json\","
        "\"state_topic\":\"%s\","
        "\"command_topic\":\"%s\","
        "\"brightness\":true,"
        "\"color_mode\":true,"
        "\"supported_color_modes\":[\"rgb\"],"
        "\"availability_topic\":\"%s\","
        "\"payload_available\":\"online\","
        "\"payload_not_available\":\"offline\","
        "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"model\":\"vanlab-iot\","
        "\"manufacturer\":\"vanlab-iot\""
        "}"
        "}",
        _deviceId, _id,
        _name,
        _stateTopic,
        _cmdTopic,
        _availTopic,
        _deviceId,
        _deviceName);

    return mqtt.publish(_discoveryTopic, payload, /*retain=*/true);
}

bool NeoPixelLightModule::publishState(MQTTClientWrapper& mqtt) {
    char payload[128];
    snprintf(payload, sizeof(payload),
        "{\"state\":\"%s\",\"color\":{\"r\":%u,\"g\":%u,\"b\":%u},\"brightness\":%u}",
        _isOn ? "ON" : "OFF",
        (unsigned)_r, (unsigned)_g, (unsigned)_b,
        (unsigned)_brightness);
    return mqtt.publish(_stateTopic, payload, /*retain=*/true);
}

void NeoPixelLightModule::resubscribe(MQTTClientWrapper& mqtt) {
    mqtt.subscribe(_cmdTopic);
}

void NeoPixelLightModule::handleCommand(const char* payload, unsigned int len) {
    if (!payload || len == 0) return;

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload, len);
    if (err) return;  // silently drop unparseable payloads

    bool stateChanged  = false;
    bool justTurnedOff = false;

    // Apply state (ON/OFF) if present
    if (doc["state"].is<const char*>()) {
        const char* state = doc["state"];
        if (state) {
            bool newOn = (strcmp(state, "ON") == 0);
            if (newOn != _isOn) {
                if (!newOn) justTurnedOff = true;
                _isOn = newOn;
                stateChanged = true;
            }
        }
    }

    // Apply color if present
    if (doc["color"].is<JsonObject>()) {
        JsonObject color = doc["color"];
        int r = color["r"] | (int)_r;
        int g = color["g"] | (int)_g;
        int b = color["b"] | (int)_b;
        // Clamp to [0, 255]
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;
        _r = (uint8_t)r;
        _g = (uint8_t)g;
        _b = (uint8_t)b;
        stateChanged = true;
    }

    // Apply brightness if present
    if (doc["brightness"].is<int>()) {
        int br = doc["brightness"];
        if (br < 0) br = 0; if (br > 255) br = 255;
        _brightness = (uint8_t)br;
        stateChanged = true;
    }

    if (!stateChanged) return;

    // Update NeoPixel hardware only if the physical output needs to change:
    //   - Light is ON (was already on, or just turned on) → show new color/brightness
    //   - Light just turned OFF → drive pixels to black
    // Brightness-only while OFF: state stored in memory, NeoPixel unchanged.
    if (_isOn || justTurnedOff) {
        applyNeoPixel();
    }
}

void NeoPixelLightModule::applyNeoPixel() {
    if (!_isOn) {
        _driver.setColor(0, 0, 0);
    } else {
        // Scale RGB by brightness (integer, rounds half-up)
        uint8_t sr = (uint8_t)(((uint16_t)_r * _brightness + 127) / 255);
        uint8_t sg = (uint8_t)(((uint16_t)_g * _brightness + 127) / 255);
        uint8_t sb = (uint8_t)(((uint16_t)_b * _brightness + 127) / 255);
        _driver.setColor(sr, sg, sb);
    }
    _driver.show();
}
