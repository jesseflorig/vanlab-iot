#include <unity.h>
#include "../../src/bundles/light/LightBundle.h"
#include "../mocks/MockGPIODriver.h"
#include "../../src/config/ConfigTypes.h"
#include "../../src/bundles/modules/physical_switch/PhysicalSwitchModule.h"
#include "../../src/mqtt/MQTTClientWrapper.h"
#include <stdint.h>

static MockGPIODriver gpio;

// The native stub (MQTTClientWrapper_native.cpp) provides no-op implementations.
// LightBundle::loop() calls (void)mqtt so no methods are actually invoked.
static DeviceConfig  s_mqttDev{};
static RuntimeConfig s_mqttRt{};
static MQTTClientWrapper fakeMqtt(s_mqttDev, s_mqttRt);

static BundleConfig makeBundleConfig() {
    BundleConfig cfg{};
    cfg.id   = "light";
    cfg.type = "light";
    cfg.name = "Main Light";
    cfg.module_count = 2;

    cfg.modules[0].id   = "dimmer";
    cfg.modules[0].type = "dimmer";
    cfg.modules[0].name = "Dimmer";
    cfg.modules[0].gpio.pins[0] = 9;
    cfg.modules[0].gpio.count   = 1;

    cfg.modules[1].id   = "switch";
    cfg.modules[1].type = "physical_switch";
    cfg.modules[1].name = "Switch";
    cfg.modules[1].gpio.pins[0] = 10;
    cfg.modules[1].gpio.count   = 1;

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

void test_setup_succeeds() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);
    TEST_ASSERT_TRUE(bundle.setup(gpio));
}

void test_get_id_returns_bundle_id() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);
    TEST_ASSERT_EQUAL_STRING("light", bundle.getId());
}

void test_availability_topic_format() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);
    TEST_ASSERT_EQUAL_STRING("vanlab/van001/availability", bundle.getAvailabilityTopic());
}

void test_handle_command_sets_level_numeric() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);
    bundle.setup(gpio);

    bundle.handleCommand("128", 3);
    bundle.loop(gpio, fakeMqtt);

    bool found = false;
    for (int i = 0; i < gpio.analogWriteCallCount(); i++) {
        if (gpio.analogWriteCall(i).pin == 9 && gpio.analogWriteCall(i).val == 128)
            found = true;
    }
    TEST_ASSERT_TRUE(found);
}

void test_handle_command_off_sets_level_zero() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);
    bundle.setup(gpio);

    bundle.handleCommand("255", 3);
    bundle.loop(gpio, fakeMqtt);
    gpio.reset();

    bundle.handleCommand("OFF", 3);
    bundle.loop(gpio, fakeMqtt);

    bool found = false;
    for (int i = 0; i < gpio.analogWriteCallCount(); i++) {
        if (gpio.analogWriteCall(i).pin == 9 && gpio.analogWriteCall(i).val == 0)
            found = true;
    }
    TEST_ASSERT_TRUE(found);
}

void test_switch_press_toggles_dimmer() {
    BundleConfig bcfg = makeBundleConfig();
    DeviceConfig dev  = makeDeviceConfig();
    LightBundle bundle(bcfg, dev);

    // Pin 10 must read HIGH (not pressed) so _stableState = HIGH after setup
    gpio.setDigitalRead(10, 1 /*HIGH*/);
    bundle.setup(gpio);

    // Turn on at 200
    bundle.handleCommand("200", 3);
    bundle.loop(gpio, fakeMqtt);

    // Reset call history but preserve pin read values
    gpio.reset();
    gpio.setDigitalRead(10, 1 /*HIGH — restore after reset*/);

    // Press the switch: pin 10 goes LOW
    gpio.setDigitalRead(10, 0 /*LOW*/);
    gpio.setMillis(0);
    bundle.loop(gpio, fakeMqtt); // pending becomes LOW, debounce starts

    gpio.setMillis(60); // past DEBOUNCE_MS (50ms)
    bundle.loop(gpio, fakeMqtt); // stable=LOW, fires SwitchPressed → toggle → _stateChanged=true

    // One more loop so DimmerModule::loop() applies the _stateChanged=true flag
    bundle.loop(gpio, fakeMqtt);

    // Dimmer should have been toggled to 0
    bool found = false;
    for (int i = 0; i < gpio.analogWriteCallCount(); i++) {
        if (gpio.analogWriteCall(i).pin == 9 && gpio.analogWriteCall(i).val == 0)
            found = true;
    }
    TEST_ASSERT_TRUE(found);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_setup_succeeds);
    RUN_TEST(test_get_id_returns_bundle_id);
    RUN_TEST(test_availability_topic_format);
    RUN_TEST(test_handle_command_sets_level_numeric);
    RUN_TEST(test_handle_command_off_sets_level_zero);
    RUN_TEST(test_switch_press_toggles_dimmer);
    return UNITY_END();
}
