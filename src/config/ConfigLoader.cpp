#include "ConfigLoader.h"
#include <ArduinoJson.h>
#include <string.h>

// LittleFS is only available on Arduino targets
#ifdef ARDUINO
#include <LittleFS.h>
#endif

bool ConfigLoader::parseFromJson(const char* json, RuntimeConfig& out) {
    if (!json) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return false;

    // mqtt_username is required
    const char* user = doc["mqtt_username"];
    if (!user) return false;

    strncpy(out.mqtt_username,   user,                          sizeof(out.mqtt_username)   - 1);
    strncpy(out.mqtt_password,   doc["mqtt_password"]   | "",   sizeof(out.mqtt_password)   - 1);
    strncpy(out.ca_cert_path,    doc["ca_cert_path"]    | "",   sizeof(out.ca_cert_path)    - 1);
    strncpy(out.client_cert_path,doc["client_cert_path"]| "",   sizeof(out.client_cert_path)- 1);
    strncpy(out.client_key_path, doc["client_key_path"] | "",   sizeof(out.client_key_path) - 1);

    out.mqtt_username[sizeof(out.mqtt_username)   - 1] = '\0';
    out.mqtt_password[sizeof(out.mqtt_password)   - 1] = '\0';
    out.ca_cert_path[sizeof(out.ca_cert_path)     - 1] = '\0';
    out.client_cert_path[sizeof(out.client_cert_path) - 1] = '\0';
    out.client_key_path[sizeof(out.client_key_path)   - 1] = '\0';

    return true;
}

#ifdef ARDUINO
bool ConfigLoader::load(RuntimeConfig& out) {
    if (!LittleFS.begin()) return false;

    File f = LittleFS.open("/config/runtime.json", "r");
    if (!f) return false;

    char buf[512];
    size_t len = f.readBytes(buf, sizeof(buf) - 1);
    buf[len] = '\0';
    f.close();

    return parseFromJson(buf, out);
}
#endif
