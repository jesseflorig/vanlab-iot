#include "HADiscoveryPayload.h"
#include "MQTTClientWrapper.h"
#include <stdio.h>
#include <string.h>

HADiscoveryPayload::HADiscoveryPayload(const char* component,
                                       const char* device_id,
                                       const char* object_id)
    : _hasCommandTopic(false)
    , _hasUnitOfMeasurement(false)
    , _hasDeviceClass(false)
    , _hasValueTemplate(false)
    , _hasDeviceInfo(false)
{
    _name[0] = _stateTopic[0] = _commandTopic[0] = _availabilityTopic[0] = '\0';
    _unitOfMeasurement[0] = _deviceClass[0] = _valueTemplate[0] = '\0';
    _deviceId[0] = _deviceName[0] = _model[0] = '\0';
    _payload[0] = '\0';

    snprintf(_topic, sizeof(_topic),
             "homeassistant/%s/%s/%s/config", component, device_id, object_id);
    snprintf(_uniqueId, sizeof(_uniqueId), "%s_%s", device_id, object_id);
}

HADiscoveryPayload& HADiscoveryPayload::setName(const char* name) {
    strncpy(_name, name, sizeof(_name) - 1);
    _name[sizeof(_name) - 1] = '\0';
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setStateTopic(const char* topic) {
    strncpy(_stateTopic, topic, sizeof(_stateTopic) - 1);
    _stateTopic[sizeof(_stateTopic) - 1] = '\0';
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setCommandTopic(const char* topic) {
    strncpy(_commandTopic, topic, sizeof(_commandTopic) - 1);
    _commandTopic[sizeof(_commandTopic) - 1] = '\0';
    _hasCommandTopic = true;
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setAvailabilityTopic(const char* topic) {
    strncpy(_availabilityTopic, topic, sizeof(_availabilityTopic) - 1);
    _availabilityTopic[sizeof(_availabilityTopic) - 1] = '\0';
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setUnitOfMeasurement(const char* unit) {
    strncpy(_unitOfMeasurement, unit, sizeof(_unitOfMeasurement) - 1);
    _unitOfMeasurement[sizeof(_unitOfMeasurement) - 1] = '\0';
    _hasUnitOfMeasurement = true;
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setDeviceClass(const char* cls) {
    strncpy(_deviceClass, cls, sizeof(_deviceClass) - 1);
    _deviceClass[sizeof(_deviceClass) - 1] = '\0';
    _hasDeviceClass = true;
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setValueTemplate(const char* tmpl) {
    strncpy(_valueTemplate, tmpl, sizeof(_valueTemplate) - 1);
    _valueTemplate[sizeof(_valueTemplate) - 1] = '\0';
    _hasValueTemplate = true;
    return *this;
}

HADiscoveryPayload& HADiscoveryPayload::setDeviceInfo(const char* device_id,
                                                       const char* device_name,
                                                       const char* model) {
    strncpy(_deviceId,   device_id,   sizeof(_deviceId)   - 1);
    strncpy(_deviceName, device_name, sizeof(_deviceName) - 1);
    strncpy(_model,      model,       sizeof(_model)      - 1);
    _deviceId[sizeof(_deviceId)   - 1] = '\0';
    _deviceName[sizeof(_deviceName) - 1] = '\0';
    _model[sizeof(_model) - 1]           = '\0';
    _hasDeviceInfo = true;
    rebuildPayload();
    return *this;
}

void HADiscoveryPayload::rebuildPayload() {
    // Build JSON manually — no ArduinoJson dependency in this class
    char buf[1024];
    int  pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos, "{");
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "\"unique_id\":\"%s\"", _uniqueId);

    if (_name[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"name\":\"%s\"", _name);

    if (_stateTopic[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"state_topic\":\"%s\"", _stateTopic);

    if (_hasCommandTopic && _commandTopic[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"command_topic\":\"%s\"", _commandTopic);

    if (_availabilityTopic[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"availability_topic\":\"%s\""
                        ",\"payload_available\":\"online\""
                        ",\"payload_not_available\":\"offline\"",
                        _availabilityTopic);

    if (_hasUnitOfMeasurement && _unitOfMeasurement[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"unit_of_measurement\":\"%s\"", _unitOfMeasurement);

    if (_hasDeviceClass && _deviceClass[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"device_class\":\"%s\"", _deviceClass);

    if (_hasValueTemplate && _valueTemplate[0])
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"value_template\":\"%s\"", _valueTemplate);

    if (_hasDeviceInfo)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",\"device\":{"
                        "\"identifiers\":[\"%s\"],"
                        "\"name\":\"%s\","
                        "\"model\":\"%s\","
                        "\"manufacturer\":\"vanlab-iot\""
                        "}",
                        _deviceId, _deviceName, _model);

    snprintf(buf + pos, sizeof(buf) - pos, "}");
    strncpy(_payload, buf, sizeof(_payload) - 1);
    _payload[sizeof(_payload) - 1] = '\0';
}

bool HADiscoveryPayload::isValid() const {
    return _uniqueId[0]          != '\0'
        && _stateTopic[0]        != '\0'
        && _availabilityTopic[0] != '\0'
        && _hasDeviceInfo;
}

bool HADiscoveryPayload::publish(MQTTClientWrapper& mqtt) const {
    if (!isValid()) return false;
    // Ensure payload is built (may not be if setDeviceInfo wasn't called last)
    const_cast<HADiscoveryPayload*>(this)->rebuildPayload();
    return mqtt.publish(_topic, _payload, true /* retain */);
}
