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

// ── Ethernet (board-specific W5500 init) ──────────────────────────────────────

#include <W5500lwIP.h>

#ifdef BOARD_FEATHER_RP2040
  // Adafruit RP2040 Feather + Silicognition PoE FeatherWing
  // SPI1: SCK=18, MOSI=19, MISO=20 — CS=10, INT=12
  static Wiznet5500lwIP eth(/*CS=*/10, SPI1, /*INT=*/12);
  static void setupSPI() {
      SPI1.setRX(20);
      SPI1.setTX(19);
      SPI1.setSCK(18);
  }
#elif defined(BOARD_RP2040_SHIM)
  // Silicognition RP2040-Shim + PoE FeatherWing (Feather D10=GPIO21, D12=GPIO14)
  // SPI (SPI0): SCK=10, MOSI=11, MISO=12 — CS=21, INT=14
  static Wiznet5500lwIP eth(/*CS=*/21, SPI, /*INT=*/14);
  static void setupSPI() {
      SPI.setRX(12);
      SPI.setTX(11);
      SPI.setSCK(10);
  }
#else
  #error "No board macro defined. Build with -DBOARD_FEATHER_RP2040 or -DBOARD_RP2040_SHIM."
#endif

// ── Hardware abstraction ──────────────────────────────────────────────────────

static RP2040GPIODriver gpio;

// ── Runtime config ────────────────────────────────────────────────────────────

static RuntimeConfig runtimeConfig;

// ── NeoPixel driver ───────────────────────────────────────────────────────────

// NeoPixel driver — pin comes from first standalone module's GPIO config
static RP2040NeoPixelDriver neoPixelDriver(
    (uint8_t)DEVICE_CONFIG.standalone_modules[0].gpio.pins[0]);

// ── NeoPixel mode selection ───────────────────────────────────────────────────
//
// Operating mode is determined at runtime from the YAML device config rather
// than a compile-time flag. A standalone module with type "neopixel_light"
// activates NeoPixelLightModule (HA RGB light). All other values use
// NeoPixelStatusModule + LightBundle (original behaviour).
//
// Bundles and modules are declared as function-local statics in setup() so
// only the active branch is ever constructed.

static bool isNeoPixelLightMode() {
    return DEVICE_CONFIG.standalone_module_count > 0
        && DEVICE_CONFIG.standalone_modules[0].type != nullptr
        && strcmp(DEVICE_CONFIG.standalone_modules[0].type, "neopixel_light") == 0;
}

// Pointer set in setup() — used by loop() to update status LED state.
static NeoPixelStatusModule* s_statusModule = nullptr;

// ── Registry & Orchestrator ───────────────────────────────────────────────────

static BundleRegistry    registry;
static MQTTClientWrapper mqtt(DEVICE_CONFIG, runtimeConfig);
static Orchestrator      orchestrator(registry, gpio, mqtt);

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // Initialize Ethernet (board-specific SPI pins + W5500)
    setupSPI();
    eth.begin();

    // Load runtime secrets from LittleFS
    if (!ConfigLoader::load(runtimeConfig)) {
        // Halt — cannot connect to broker without credentials
        while (true) delay(1000);
    }

    // Validate GPIO pins against the board's legal pin set
    if (!ConfigLoader::validatePins(DEVICE_CONFIG)) {
        Serial.println("ERROR: Device config contains invalid GPIO pin for this board. Halting.");
        while (true) delay(1000);
    }

    if (isNeoPixelLightMode()) {
        static NeoPixelLightModule neoPixelLight(
            neoPixelDriver, DEVICE_CONFIG.standalone_modules[0], DEVICE_CONFIG);
        registry.registerModule(&neoPixelLight);
        mqtt.setMessageCallback([](char* topic, uint8_t* payload, unsigned int len) {
            (void)topic;  // only one subscribed topic in this config
            neoPixelLight.handleCommand((const char*)payload, len);
        });
    } else {
        static LightBundle lightBundle(DEVICE_CONFIG.bundles[0], DEVICE_CONFIG);
        static NeoPixelStatusModule neoPixelStatus(
            neoPixelDriver, DEVICE_CONFIG.standalone_modules[0], DEVICE_CONFIG);
        s_statusModule = &neoPixelStatus;
        registry.registerBundle(&lightBundle);
        registry.registerModule(&neoPixelStatus);
    }

    orchestrator.setup();
}

void loop() {
    if (s_statusModule) {
        // Reflect MQTT connection state on the onboard NeoPixel status indicator
        switch (mqtt.state()) {
            case MQTTClientWrapper::ConnState::Connected:
                s_statusModule->setState(NeoPixelStatusModule::StatusState::Connected);
                break;
            default:
                s_statusModule->setState(NeoPixelStatusModule::StatusState::Connecting);
                break;
        }
    }

    orchestrator.loop();
}
