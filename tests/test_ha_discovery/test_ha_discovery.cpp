#include <unity.h>
#include "../../src/mqtt/HADiscoveryPayload.h"
#include <string.h>

void setUp() {}
void tearDown() {}

void test_topic_format() {
    HADiscoveryPayload p("light", "my_device", "light_1");
    TEST_ASSERT_EQUAL_STRING(
        "homeassistant/light/my_device/light_1/config",
        p.getTopic());
}

void test_is_not_valid_without_required_fields() {
    HADiscoveryPayload p("light", "dev", "obj");
    TEST_ASSERT_FALSE(p.isValid());
}

void test_is_valid_with_all_required_fields() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("home/dev/obj/state")
     .setAvailabilityTopic("home/dev/availability")
     .setDeviceInfo("dev", "My Device", "vanlab-iot");

    TEST_ASSERT_TRUE(p.isValid());
}

void test_is_not_valid_missing_state_topic() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setAvailabilityTopic("home/dev/availability")
     .setDeviceInfo("dev", "My Device", "vanlab-iot");
    TEST_ASSERT_FALSE(p.isValid());
}

void test_is_not_valid_missing_availability_topic() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("home/dev/obj/state")
     .setDeviceInfo("dev", "My Device", "vanlab-iot");
    TEST_ASSERT_FALSE(p.isValid());
}

void test_is_not_valid_missing_device_info() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("home/dev/obj/state")
     .setAvailabilityTopic("home/dev/availability");
    TEST_ASSERT_FALSE(p.isValid());
}

void test_payload_contains_unique_id() {
    HADiscoveryPayload p("sensor", "dev1", "temp");
    p.setStateTopic("s")
     .setAvailabilityTopic("a")
     .setDeviceInfo("dev1", "Dev", "model");

    const char* payload = p.getPayload();
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"unique_id\":\"dev1_temp\""));
}

void test_payload_contains_state_topic() {
    HADiscoveryPayload p("sensor", "dev", "obj");
    p.setStateTopic("home/dev/obj/state")
     .setAvailabilityTopic("avail")
     .setDeviceInfo("dev", "D", "m");

    TEST_ASSERT_NOT_NULL(strstr(p.getPayload(), "\"state_topic\":\"home/dev/obj/state\""));
}

void test_payload_contains_command_topic_when_set() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("s")
     .setCommandTopic("home/dev/obj/command")
     .setAvailabilityTopic("a")
     .setDeviceInfo("dev", "D", "m");

    TEST_ASSERT_NOT_NULL(strstr(p.getPayload(), "\"command_topic\":\"home/dev/obj/command\""));
}

void test_payload_omits_command_topic_when_not_set() {
    HADiscoveryPayload p("sensor", "dev", "obj");
    p.setStateTopic("s")
     .setAvailabilityTopic("a")
     .setDeviceInfo("dev", "D", "m");

    TEST_ASSERT_NULL(strstr(p.getPayload(), "command_topic"));
}

void test_payload_contains_availability_topic() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("s")
     .setAvailabilityTopic("home/dev/availability")
     .setDeviceInfo("dev", "D", "m");

    TEST_ASSERT_NOT_NULL(strstr(p.getPayload(), "\"availability_topic\":\"home/dev/availability\""));
}

void test_payload_contains_device_block() {
    HADiscoveryPayload p("light", "dev", "obj");
    p.setStateTopic("s")
     .setAvailabilityTopic("a")
     .setDeviceInfo("dev", "My Light", "vanlab-iot");

    const char* payload = p.getPayload();
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"device\":"));
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"name\":\"My Light\""));
}

void test_builder_returns_self_for_chaining() {
    HADiscoveryPayload p("light", "dev", "obj");
    HADiscoveryPayload& ref = p.setName("Test");
    TEST_ASSERT_EQUAL_PTR(&p, &ref);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_topic_format);
    RUN_TEST(test_is_not_valid_without_required_fields);
    RUN_TEST(test_is_valid_with_all_required_fields);
    RUN_TEST(test_is_not_valid_missing_state_topic);
    RUN_TEST(test_is_not_valid_missing_availability_topic);
    RUN_TEST(test_is_not_valid_missing_device_info);
    RUN_TEST(test_payload_contains_unique_id);
    RUN_TEST(test_payload_contains_state_topic);
    RUN_TEST(test_payload_contains_command_topic_when_set);
    RUN_TEST(test_payload_omits_command_topic_when_not_set);
    RUN_TEST(test_payload_contains_availability_topic);
    RUN_TEST(test_payload_contains_device_block);
    RUN_TEST(test_builder_returns_self_for_chaining);
    return UNITY_END();
}
