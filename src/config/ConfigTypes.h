#pragma once
#include <stdint.h>

// ── Size limits ──────────────────────────────────────────────────────────────
static constexpr uint8_t MAX_BUNDLES            =  8;
static constexpr uint8_t MAX_MODULES            = 16;
static constexpr uint8_t MAX_MODULES_PER_BUNDLE =  8;
static constexpr uint8_t MAX_GPIO_PER_MODULE    =  4;
static constexpr uint8_t MAX_MODULE_PARAMS      =  8;

// ── GPIO ─────────────────────────────────────────────────────────────────────

struct GpioConfig {
    int8_t      pins[MAX_GPIO_PER_MODULE];   // -1 = not assigned
    const char* roles[MAX_GPIO_PER_MODULE];  // e.g. "control", "data", "feedback"
    uint8_t     count;
};

// ── Module ───────────────────────────────────────────────────────────────────

struct ModuleConfig {
    const char*  type;                               // e.g. "dimmer", "status_led"
    const char*  id;                                 // unique within bundle or device
    const char*  name;                               // human-readable
    const char*  topic_prefix;                       // MQTT topic prefix
    uint32_t     poll_interval_ms;                   // 0 = event-driven
    GpioConfig   gpio;
    const char*  params[MAX_MODULE_PARAMS][2];       // key/value pairs
    uint8_t      param_count;
};

// ── Bundle ───────────────────────────────────────────────────────────────────

struct BundleConfig {
    const char*  type;                               // e.g. "light", "environment"
    const char*  id;                                 // unique within device
    const char*  name;                               // human-readable
    const char*  topic_prefix;
    ModuleConfig modules[MAX_MODULES_PER_BUNDLE];
    uint8_t      module_count;
    const char*  params[MAX_MODULE_PARAMS][2];       // bundle-level key/value pairs
    uint8_t      param_count;
};

// ── MQTT ─────────────────────────────────────────────────────────────────────

struct MQTTConfig {
    const char* broker_host;
    uint16_t    broker_port;      // default: 8883
    const char* client_id;        // defaults to device_id if null
    uint16_t    keepalive_s;      // default: 60
    uint16_t    socket_timeout_s; // default: 15
    const char* topic_root;       // default: "vanlab/<device_id>"
};

// ── Device ───────────────────────────────────────────────────────────────────

struct DeviceConfig {
    const char*  device_id;
    const char*  device_name;
    const char*  board;
    MQTTConfig   mqtt;
    BundleConfig bundles[MAX_BUNDLES];
    uint8_t      bundle_count;
    ModuleConfig standalone_modules[MAX_MODULES];
    uint8_t      standalone_module_count;
};

// ── Runtime (loaded from LittleFS /config/runtime.json) ──────────────────────

struct RuntimeConfig {
    char mqtt_username[64];
    char mqtt_password[64];
    char ca_cert_path[64];
    char client_cert_path[64];  // optional — mTLS
    char client_key_path[64];   // optional — mTLS
};
