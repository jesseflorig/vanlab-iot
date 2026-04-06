#include <unity.h>
#include "../../src/bundles/modules/neopixel/NeoPixelStatusModule.h"
#include "../mocks/MockNeoPixelDriver.h"
#include "../mocks/MockGPIODriver.h"
#include "../../src/config/ConfigTypes.h"

static MockNeoPixelDriver neoDriver;
static MockGPIODriver     gpio;

static ModuleConfig makeModuleConfig(uint8_t pin) {
    ModuleConfig cfg{};
    cfg.id   = "neopixel_status";
    cfg.type = "neopixel_status";
    cfg.name = "Status";
    cfg.gpio.pins[0] = (int8_t)pin;
    cfg.gpio.count   = 1;
    return cfg;
}

static DeviceConfig makeDeviceConfig() {
    DeviceConfig dev{};
    dev.device_id   = "van001";
    dev.device_name = "Van";
    dev.mqtt.topic_root = "vanlab/van001";
    return dev;
}

void setUp() {
    neoDriver.reset();
    gpio.reset();
}

void tearDown() {}

// ── setup() ──────────────────────────────────────────────────────────────────

void test_setup_calls_driver_begin() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);

    TEST_ASSERT_FALSE(neoDriver.beginCalled);
    module.setup(gpio);
    TEST_ASSERT_TRUE(neoDriver.beginCalled);
}

// ── Connected state → green ───────────────────────────────────────────────────

void test_connected_state_shows_green() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);
    module.setup(gpio);

    module.setState(NeoPixelStatusModule::StatusState::Connected);
    // Advance time past the toggle interval so the pixel turns on
    gpio.setMillis(501);
    module.loop(gpio);

    TEST_ASSERT_EQUAL(0,  neoDriver.lastR);
    TEST_ASSERT_TRUE(neoDriver.lastG > 0);
    TEST_ASSERT_EQUAL(0,  neoDriver.lastB);
}

// ── Connecting state → yellow ─────────────────────────────────────────────────

void test_connecting_state_shows_yellow() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);
    module.setup(gpio);

    // Default state is Connecting; advance past the 125ms interval
    gpio.setMillis(126);
    module.loop(gpio);

    TEST_ASSERT_TRUE(neoDriver.lastR > 0);
    TEST_ASSERT_TRUE(neoDriver.lastG > 0);
    TEST_ASSERT_EQUAL(0, neoDriver.lastB);
}

// ── Error state → red solid ───────────────────────────────────────────────────

void test_error_state_shows_red() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);
    module.setup(gpio);

    module.setState(NeoPixelStatusModule::StatusState::Error);
    gpio.setMillis(0);
    module.loop(gpio);

    TEST_ASSERT_TRUE(neoDriver.lastR > 0);
    TEST_ASSERT_EQUAL(0, neoDriver.lastG);
    TEST_ASSERT_EQUAL(0, neoDriver.lastB);
}

void test_error_state_does_not_blink() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);
    module.setup(gpio);

    module.setState(NeoPixelStatusModule::StatusState::Error);
    gpio.setMillis(0);
    module.loop(gpio); // first call — shows red, showCount = 1
    uint8_t firstCount = neoDriver.showCount;

    // Advance time well past any blink interval — should not show again
    gpio.setMillis(5000);
    module.loop(gpio);
    gpio.setMillis(10000);
    module.loop(gpio);

    TEST_ASSERT_EQUAL(firstCount, neoDriver.showCount);
}

// ── show() called on each state update ───────────────────────────────────────

void test_loop_calls_show_when_toggling() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);
    module.setup(gpio);

    module.setState(NeoPixelStatusModule::StatusState::Connected);
    gpio.setMillis(0);
    module.loop(gpio); // t=0: no toggle yet (interval not elapsed)

    gpio.setMillis(501); // past 500ms interval
    module.loop(gpio);   // should toggle → show() called
    TEST_ASSERT_TRUE(neoDriver.showCount > 0);
}

// ── getId / getAvailabilityTopic ──────────────────────────────────────────────

void test_get_id_returns_config_id() {
    ModuleConfig mod = makeModuleConfig(16);
    DeviceConfig dev = makeDeviceConfig();
    NeoPixelStatusModule module(neoDriver, mod, dev);

    TEST_ASSERT_EQUAL_STRING("neopixel_status", module.getId());
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(test_setup_calls_driver_begin);
    RUN_TEST(test_connected_state_shows_green);
    RUN_TEST(test_connecting_state_shows_yellow);
    RUN_TEST(test_error_state_shows_red);
    RUN_TEST(test_error_state_does_not_blink);
    RUN_TEST(test_loop_calls_show_when_toggling);
    RUN_TEST(test_get_id_returns_config_id);

    return UNITY_END();
}
