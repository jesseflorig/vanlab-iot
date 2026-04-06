#pragma once
#include <stdint.h>

class MQTTClientWrapper;

/**
 * HADiscoveryPayload — builder for Home Assistant MQTT Discovery payloads.
 *
 * Constructs the JSON payload and topic path for a given entity. Every bundle
 * uses this to register its entities with HA on each MQTT connection.
 *
 * All payloads MUST be published with retain=true.
 * See contracts/ha-discovery.md for required fields and format.
 */
class HADiscoveryPayload {
public:
    /**
     * @param component  HA entity type, e.g. "sensor", "switch", "light"
     * @param device_id  DeviceConfig.device_id
     * @param object_id  ModuleConfig.id (or bundle id for unified entities)
     */
    HADiscoveryPayload(const char* component,
                       const char* device_id,
                       const char* object_id);

    HADiscoveryPayload& setName(const char* name);
    HADiscoveryPayload& setStateTopic(const char* topic);
    HADiscoveryPayload& setCommandTopic(const char* topic);
    HADiscoveryPayload& setAvailabilityTopic(const char* topic);
    HADiscoveryPayload& setUnitOfMeasurement(const char* unit);
    HADiscoveryPayload& setDeviceClass(const char* cls);
    HADiscoveryPayload& setValueTemplate(const char* tmpl);
    HADiscoveryPayload& setDeviceInfo(const char* device_id,
                                       const char* device_name,
                                       const char* model);

    // homeassistant/<component>/<device_id>/<object_id>/config
    const char* getTopic() const   { return _topic; }

    // JSON payload string
    const char* getPayload() const { return _payload; }

    // Validates required fields: unique_id, state_topic, availability_topic, device
    bool isValid() const;

    // Publish this payload via the given MQTT client
    bool publish(MQTTClientWrapper& mqtt) const;

private:
    void rebuildPayload();

    char _topic[256];
    char _payload[1024];

    // Field storage
    char _name[64];
    char _stateTopic[128];
    char _commandTopic[128];
    char _availabilityTopic[128];
    char _unitOfMeasurement[32];
    char _deviceClass[32];
    char _valueTemplate[64];
    char _deviceId[64];
    char _deviceName[64];
    char _model[64];
    char _uniqueId[128];

    bool _hasCommandTopic;
    bool _hasUnitOfMeasurement;
    bool _hasDeviceClass;
    bool _hasValueTemplate;
    bool _hasDeviceInfo;
};
