/**
 * main.cpp — vanlab-iot Arduino entry point.
 *
 * Wires together the generated DeviceConfig (from scripts/gen_config.py),
 * the RuntimeConfig (loaded from LittleFS /config/runtime.json), the
 * Orchestrator, and the application bundles.
 *
 * To add bundles or standalone modules:
 *  1. Declare them as static globals below.
 *  2. Register them in setup() via registry.registerBundle() or
 *     registry.registerModule().
 */

#include "vanlab-iot.h"

// Generated at build time by scripts/gen_config.py
// Defines: extern const DeviceConfig DEVICE_CONFIG;
#if __has_include("generated_config.h")
#include "generated_config.h"
#else
// Fallback stub for builds without the codegen script
static const DeviceConfig DEVICE_CONFIG{};
#endif

// ── Hardware abstraction ──────────────────────────────────────────────────────

static RP2040GPIODriver gpio;

// ── Runtime config ────────────────────────────────────────────────────────────

static RuntimeConfig runtimeConfig;

// ── Bundles ───────────────────────────────────────────────────────────────────

// LightBundle is registered if the device config contains a "light" bundle.
// For single-bundle devices, declare it directly:
static LightBundle lightBundle(DEVICE_CONFIG.bundles[0], DEVICE_CONFIG);

// ── Standalone modules ────────────────────────────────────────────────────────

static StatusLEDModule statusLed(DEVICE_CONFIG.standalone_modules[0], DEVICE_CONFIG);

// ── Registry & Orchestrator ───────────────────────────────────────────────────

static BundleRegistry    registry;
static MQTTClientWrapper mqtt(DEVICE_CONFIG, runtimeConfig);
static Orchestrator      orchestrator(registry, gpio, mqtt);

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // Load runtime secrets from LittleFS
    if (!ConfigLoader::load(runtimeConfig)) {
        // Halt — cannot connect to broker without credentials
        while (true) delay(1000);
    }

    registry.registerBundle(&lightBundle);
    registry.registerModule(&statusLed);

    orchestrator.setup();
}

void loop() {
    orchestrator.loop();
}
