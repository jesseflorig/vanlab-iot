#include "ConfigLoader.h"
#include <ArduinoJson.h>
#include <string.h>

// LittleFS is only available on Arduino targets
#ifdef ARDUINO
#include <LittleFS.h>
#endif

// ── Board pin set definitions ─────────────────────────────────────────────────

#ifdef BOARD_FEATHER_RP2040
static const uint8_t BOARD_VALID_PINS[] = {
    0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13,
    16,           // NeoPixel (internal, not on Feather headers)
    18, 19, 20,   // SPI1 (shared with W5500 SCK/MOSI/MISO)
    24, 25,
    26, 27, 28, 29
};
static const uint8_t BOARD_VALID_PINS_COUNT =
    sizeof(BOARD_VALID_PINS) / sizeof(BOARD_VALID_PINS[0]);
#elif defined(BOARD_RP2040_SHIM)
// Silicognition RP2040-Shim exposes all standard RP2040 GPIOs 0–29
static const uint8_t BOARD_VALID_PINS[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29
};
static const uint8_t BOARD_VALID_PINS_COUNT =
    sizeof(BOARD_VALID_PINS) / sizeof(BOARD_VALID_PINS[0]);
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

bool ConfigLoader::validatePins(const DeviceConfig& cfg) {
#if defined(BOARD_FEATHER_RP2040) || defined(BOARD_RP2040_SHIM)
    // Helper: returns true if pin is in BOARD_VALID_PINS
    auto isValid = [](int8_t pin) -> bool {
        if (pin < 0) return true; // -1 = not assigned
        for (uint8_t i = 0; i < BOARD_VALID_PINS_COUNT; i++) {
            if (BOARD_VALID_PINS[i] == (uint8_t)pin) return true;
        }
        return false;
    };

    // Check all standalone module GPIO pins
    for (uint8_t m = 0; m < cfg.standalone_module_count; m++) {
        const GpioConfig& gpio = cfg.standalone_modules[m].gpio;
        for (uint8_t p = 0; p < gpio.count; p++) {
            if (!isValid(gpio.pins[p])) return false;
        }
    }

    // Check all bundle module GPIO pins
    for (uint8_t b = 0; b < cfg.bundle_count; b++) {
        for (uint8_t m = 0; m < cfg.bundles[b].module_count; m++) {
            const GpioConfig& gpio = cfg.bundles[b].modules[m].gpio;
            for (uint8_t p = 0; p < gpio.count; p++) {
                if (!isValid(gpio.pins[p])) return false;
            }
        }
    }
#endif
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
