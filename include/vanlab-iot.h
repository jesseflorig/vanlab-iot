#pragma once

/**
 * vanlab-iot — framework entry point.
 *
 * Include this single header in device sketches to pull in all public
 * interfaces, configuration types, and the Orchestrator.
 */

// ── Core interfaces ───────────────────────────────────────────────────────────
#include "IModule.h"
#include "IBundle.h"
#include "IGPIODriver.h"

// ── Configuration types ───────────────────────────────────────────────────────
#include "../src/config/ConfigTypes.h"

// ── HAL ───────────────────────────────────────────────────────────────────────
#ifdef ARDUINO
#include "../src/hal/RP2040GPIODriver.h"
#endif

// ── Orchestrator ──────────────────────────────────────────────────────────────
#include "../src/core/Orchestrator.h"

// ── Registry ──────────────────────────────────────────────────────────────────
#include "../src/bundles/base/BundleRegistry.h"

// ── Config ────────────────────────────────────────────────────────────────────
#include "../src/config/ConfigLoader.h"

// ── MQTT ──────────────────────────────────────────────────────────────────────
#include "../src/mqtt/MQTTClientWrapper.h"
#include "../src/mqtt/StateBuffer.h"
#include "../src/mqtt/HADiscoveryPayload.h"

// ── Bundles ───────────────────────────────────────────────────────────────────
#include "../src/bundles/light/LightBundle.h"

// ── Standalone modules ────────────────────────────────────────────────────────
#include "../src/bundles/modules/status_led/StatusLEDModule.h"
