#pragma once
#include "ConfigTypes.h"

/**
 * ConfigLoader — loads RuntimeConfig from LittleFS JSON at boot.
 *
 * The static parseFromJson() method is platform-independent and used
 * in native unit tests. The load() method depends on LittleFS and is
 * excluded from the native build via build_src_filter.
 */
class ConfigLoader {
public:
    /**
     * Parse a RuntimeConfig from a JSON string.
     * Used directly in tests (no filesystem dependency).
     * Returns false if JSON is malformed or required fields are missing.
     */
    static bool parseFromJson(const char* json, RuntimeConfig& out);

    /**
     * Validate that all GPIO pins in the DeviceConfig are within the
     * board's legal pin set (determined at compile time via build flags).
     * Returns false if any pin is out of range for the target board.
     * Always returns true on native builds (no board macro defined).
     */
    static bool validatePins(const DeviceConfig& cfg);

#ifdef ARDUINO
    /**
     * Load RuntimeConfig from /config/runtime.json on LittleFS.
     * Returns false if the file is missing or the JSON is invalid.
     */
    static bool load(RuntimeConfig& out);
#endif
};
