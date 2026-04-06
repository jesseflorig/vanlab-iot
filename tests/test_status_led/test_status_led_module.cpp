#include <unity.h>
#include "../../src/bundles/modules/status_led/StatusLEDModule.h"
#include "../mocks/MockGPIODriver.h"
#include "../../src/config/ConfigTypes.h"

static MockGPIODriver gpio;

static ModuleConfig makeModuleConfig(uint8_t pin) {
    ModuleConfig cfg{};
    cfg.id   = "status_led";
    cfg.type = "status_led";
    cfg.name = "Status LED";
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
    gpio.reset();
}

void tearDown() {}

void test_setup_sets_pin_mode_output() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);

    TEST_ASSERT_TRUE(led.setup(gpio));
    TEST_ASSERT_EQUAL(1, gpio.pinModeCallCount());
    TEST_ASSERT_EQUAL(25, gpio.pinModeCall(0).pin);
    TEST_ASSERT_EQUAL(1 /*OUTPUT*/, gpio.pinModeCall(0).mode);
}

void test_setup_writes_low_initially() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);
    led.setup(gpio);

    TEST_ASSERT_EQUAL(1, gpio.digitalWriteCallCount());
    TEST_ASSERT_EQUAL(0 /*LOW*/, gpio.digitalWriteCall(0).val);
}

void test_loop_toggles_after_blink_interval() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);
    led.setup(gpio);

    gpio.setMillis(0);
    led.loop(gpio);
    int writesAtZero = gpio.digitalWriteCallCount();

    gpio.setMillis(501);
    led.loop(gpio);
    TEST_ASSERT_GREATER_THAN(writesAtZero, gpio.digitalWriteCallCount());
}

void test_loop_does_not_toggle_before_interval() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);
    led.setup(gpio);

    gpio.setMillis(0);
    led.loop(gpio);
    int count = gpio.digitalWriteCallCount();

    gpio.setMillis(100);
    led.loop(gpio);
    TEST_ASSERT_EQUAL(count, gpio.digitalWriteCallCount());
}

void test_get_id_returns_configured_id() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);
    TEST_ASSERT_EQUAL_STRING("status_led", led.getId());
}

void test_availability_topic_format() {
    ModuleConfig mod = makeModuleConfig(25);
    DeviceConfig dev = makeDeviceConfig();
    StatusLEDModule led(mod, dev);
    TEST_ASSERT_EQUAL_STRING("vanlab/van001/availability", led.getAvailabilityTopic());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_setup_sets_pin_mode_output);
    RUN_TEST(test_setup_writes_low_initially);
    RUN_TEST(test_loop_toggles_after_blink_interval);
    RUN_TEST(test_loop_does_not_toggle_before_interval);
    RUN_TEST(test_get_id_returns_configured_id);
    RUN_TEST(test_availability_topic_format);
    return UNITY_END();
}
