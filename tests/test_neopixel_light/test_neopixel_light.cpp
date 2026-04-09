#include <unity.h>
#include "../../src/bundles/modules/neopixel/NeoPixelLightModule.h"
#include "../mocks/MockNeoPixelDriver.h"
#include "../mocks/MockGPIODriver.h"
#include "../../src/config/ConfigTypes.h"
#include <string.h>

static MockNeoPixelDriver neoDriver;
static MockGPIODriver     gpio;

static ModuleConfig makeModuleConfig() {
    ModuleConfig cfg{};
    cfg.id   = "rgb_light";
    cfg.type = "neopixel_light";
    cfg.name = "RGB Light";
    cfg.gpio.pins[0] = 16;
    cfg.gpio.count   = 1;
    return cfg;
}

static DeviceConfig makeDeviceConfig() {
    DeviceConfig dev{};
    dev.device_id   = "feather-rgb-light";
    dev.device_name = "Feather RGB Light";
    dev.mqtt.topic_root = "vanlab/feather-rgb-light";
    return dev;
}

void setUp() {
    neoDriver.reset();
    gpio.reset();
}

void tearDown() {}

// ── setup() ──────────────────────────────────────────────────────────────────

void test_setup_calls_driver_begin() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);

    TEST_ASSERT_FALSE(neoDriver.beginCalled);
    module.setup(gpio);
    TEST_ASSERT_TRUE(neoDriver.beginCalled);
}

// ── getId / getAvailabilityTopic ──────────────────────────────────────────────

void test_get_id_returns_config_id() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);

    TEST_ASSERT_EQUAL_STRING("rgb_light", module.getId());
}

void test_get_availability_topic() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);

    TEST_ASSERT_EQUAL_STRING("vanlab/feather-rgb-light/availability",
                             module.getAvailabilityTopic());
}

// ── handleCommand: OFF ────────────────────────────────────────────────────────

void test_command_off_turns_neopixel_off() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // First turn on
    const char* on = "{\"state\":\"ON\",\"color\":{\"r\":255,\"g\":0,\"b\":0}}";
    module.handleCommand(on, strlen(on));
    neoDriver.reset();

    // Now turn off
    const char* off = "{\"state\":\"OFF\"}";
    module.handleCommand(off, strlen(off));

    TEST_ASSERT_EQUAL(0, neoDriver.lastR);
    TEST_ASSERT_EQUAL(0, neoDriver.lastG);
    TEST_ASSERT_EQUAL(0, neoDriver.lastB);
    TEST_ASSERT_TRUE(neoDriver.showCount > 0);
}

// ── handleCommand: ON with RGB color ─────────────────────────────────────────

void test_command_on_with_red_shows_red() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":255,\"g\":0,\"b\":0},\"brightness\":255}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(255, neoDriver.lastR);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastG);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastB);
    TEST_ASSERT_TRUE(neoDriver.showCount > 0);
}

void test_command_on_with_blue_shows_blue() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":0,\"g\":0,\"b\":200},\"brightness\":255}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(0,   neoDriver.lastR);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastG);
    TEST_ASSERT_EQUAL(200, neoDriver.lastB);
}

// ── handleCommand: ON without color → last or white default ──────────────────

void test_command_on_without_color_restores_last_color() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // Set a color first, then turn off
    const char* setRed = "{\"state\":\"ON\",\"color\":{\"r\":200,\"g\":0,\"b\":0}}";
    module.handleCommand(setRed, strlen(setRed));
    const char* off = "{\"state\":\"OFF\"}";
    module.handleCommand(off, strlen(off));
    neoDriver.reset();

    // ON without color — should restore last (red)
    const char* on = "{\"state\":\"ON\"}";
    module.handleCommand(on, strlen(on));

    TEST_ASSERT_EQUAL(200, neoDriver.lastR);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastG);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastB);
}

void test_command_on_without_prior_color_defaults_to_white() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // ON on a fresh module with no prior color command
    const char* on = "{\"state\":\"ON\"}";
    module.handleCommand(on, strlen(on));

    // Default color is white (255,255,255)
    TEST_ASSERT_EQUAL(255, neoDriver.lastR);
    TEST_ASSERT_EQUAL(255, neoDriver.lastG);
    TEST_ASSERT_EQUAL(255, neoDriver.lastB);
}

// ── handleCommand: brightness scaling ────────────────────────────────────────

void test_brightness_scales_rgb_uniformly() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // Full red at half brightness: (255 * 128 + 127) / 255 = 128
    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":255,\"g\":0,\"b\":0},\"brightness\":128}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(128, neoDriver.lastR);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastG);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastB);
}

void test_brightness_zero_shows_off() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":255,\"g\":255,\"b\":255},\"brightness\":0}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(0, neoDriver.lastR);
    TEST_ASSERT_EQUAL(0, neoDriver.lastG);
    TEST_ASSERT_EQUAL(0, neoDriver.lastB);
}

// ── handleCommand: out-of-range clamping ──────────────────────────────────────

void test_out_of_range_rgb_is_clamped() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // r=300 (over), g=-5 (under) — JSON integers, ArduinoJson parses as int
    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":300,\"g\":-5,\"b\":128},\"brightness\":255}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(255, neoDriver.lastR);  // clamped from 300
    TEST_ASSERT_EQUAL(0,   neoDriver.lastG);  // clamped from -5
    TEST_ASSERT_EQUAL(128, neoDriver.lastB);
}

// ── handleCommand: brightness-only while off ──────────────────────────────────

void test_brightness_only_while_off_does_not_turn_on() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // Module starts off; send brightness only
    const char* cmd = "{\"brightness\":200}";
    module.handleCommand(cmd, strlen(cmd));

    // NeoPixel should NOT have been updated (still off)
    TEST_ASSERT_EQUAL(0, neoDriver.showCount);
}

void test_brightness_stored_for_next_on() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // Pre-set brightness while off
    const char* setBr = "{\"brightness\":128}";
    module.handleCommand(setBr, strlen(setBr));

    // Turn on with full red — brightness should be applied
    const char* on = "{\"state\":\"ON\",\"color\":{\"r\":255,\"g\":0,\"b\":0}}";
    module.handleCommand(on, strlen(on));

    // Scaled: (255 * 128 + 127) / 255 = 128
    TEST_ASSERT_EQUAL(128, neoDriver.lastR);
}

// ── handleCommand: show() called on each change ───────────────────────────────

void test_show_called_on_state_change() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":100,\"g\":100,\"b\":100}}";
    module.handleCommand(cmd, strlen(cmd));

    TEST_ASSERT_EQUAL(1, neoDriver.showCount);

    const char* off = "{\"state\":\"OFF\"}";
    module.handleCommand(off, strlen(off));

    TEST_ASSERT_EQUAL(2, neoDriver.showCount);
}

// ── handleCommand: invalid payload ───────────────────────────────────────────

void test_invalid_json_is_silently_ignored() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* bad = "not json at all {{{{";
    module.handleCommand(bad, strlen(bad));

    // No NeoPixel calls — payload dropped silently
    TEST_ASSERT_EQUAL(0, neoDriver.showCount);
}

void test_empty_payload_is_silently_ignored() {
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    module.handleCommand(nullptr, 0);
    module.handleCommand("", 0);

    TEST_ASSERT_EQUAL(0, neoDriver.showCount);
}

// ── Reconnect: state retained, no NeoPixel clear ─────────────────────────────

void test_neopixel_state_retained_without_mqtt() {
    // NeoPixelLightModule holds state in memory — loop() and resubscribe() must
    // not touch the NeoPixel. The pixel retains its last state during disconnect.
    ModuleConfig mod = makeModuleConfig();
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelLightModule module(neoDriver, mod, dev);
    module.setup(gpio);

    const char* cmd = "{\"state\":\"ON\",\"color\":{\"r\":0,\"g\":200,\"b\":0}}";
    module.handleCommand(cmd, strlen(cmd));

    uint8_t showAfterCommand = neoDriver.showCount;

    // Simulate loop() ticks (no MQTT — loop is a no-op)
    module.loop(gpio);
    module.loop(gpio);
    module.loop(gpio);

    // No additional show() calls — NeoPixel state unchanged
    TEST_ASSERT_EQUAL(showAfterCommand, neoDriver.showCount);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastR);
    TEST_ASSERT_EQUAL(200, neoDriver.lastG);
    TEST_ASSERT_EQUAL(0,   neoDriver.lastB);
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(test_setup_calls_driver_begin);
    RUN_TEST(test_get_id_returns_config_id);
    RUN_TEST(test_get_availability_topic);

    RUN_TEST(test_command_off_turns_neopixel_off);
    RUN_TEST(test_command_on_with_red_shows_red);
    RUN_TEST(test_command_on_with_blue_shows_blue);

    RUN_TEST(test_command_on_without_color_restores_last_color);
    RUN_TEST(test_command_on_without_prior_color_defaults_to_white);

    RUN_TEST(test_brightness_scales_rgb_uniformly);
    RUN_TEST(test_brightness_zero_shows_off);

    RUN_TEST(test_out_of_range_rgb_is_clamped);

    RUN_TEST(test_brightness_only_while_off_does_not_turn_on);
    RUN_TEST(test_brightness_stored_for_next_on);

    RUN_TEST(test_show_called_on_state_change);

    RUN_TEST(test_invalid_json_is_silently_ignored);
    RUN_TEST(test_empty_payload_is_silently_ignored);

    RUN_TEST(test_neopixel_state_retained_without_mqtt);

    return UNITY_END();
}
