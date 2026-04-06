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

// ── Bundles ───────────────────────────────────────────────────────────────────

// LightBundle is registered if the device config contains a "light" bundle.
// For single-bundle devices, declare it directly:
static LightBundle lightBundle(DEVICE_CONFIG.bundles[0], DEVICE_CONFIG);

// ── Standalone modules ────────────────────────────────────────────────────────

// NeoPixel driver uses the pin from the first standalone module's GPIO config
static RP2040NeoPixelDriver neoPixelDriver(
    (uint8_t)DEVICE_CONFIG.standalone_modules[0].gpio.pins[0]);
static NeoPixelStatusModule neoPixelStatus(
    neoPixelDriver, DEVICE_CONFIG.standalone_modules[0], DEVICE_CONFIG);

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

    registry.registerBundle(&lightBundle);
    registry.registerModule(&neoPixelStatus);

    orchestrator.setup();
}

void loop() {
    // Reflect MQTT connection state on the onboard NeoPixel
    switch (mqtt.state()) {
        case MQTTClientWrapper::ConnState::Connected:
            neoPixelStatus.setState(NeoPixelStatusModule::StatusState::Connected);
            break;
        default:
            neoPixelStatus.setState(NeoPixelStatusModule::StatusState::Connecting);
            break;
    }

    orchestrator.loop();
}
