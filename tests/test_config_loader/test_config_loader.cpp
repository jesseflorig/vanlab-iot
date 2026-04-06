#include <unity.h>
#include "../../src/config/ConfigLoader.h"
#include "../../src/config/ConfigTypes.h"
#include <string.h>

static RuntimeConfig cfg;

void setUp() {
    cfg = RuntimeConfig{};
}

void tearDown() {}

void test_parse_minimal_valid_json() {
    const char* json = "{\"mqtt_username\":\"user1\"}";
    TEST_ASSERT_TRUE(ConfigLoader::parseFromJson(json, cfg));
    TEST_ASSERT_EQUAL_STRING("user1", cfg.mqtt_username);
    TEST_ASSERT_EQUAL_STRING("", cfg.mqtt_password);
}

void test_parse_full_json() {
    const char* json =
        "{"
        "\"mqtt_username\":\"admin\","
        "\"mqtt_password\":\"secret\","
        "\"ca_cert_path\":\"/certs/ca.crt\","
        "\"client_cert_path\":\"/certs/client.crt\","
        "\"client_key_path\":\"/certs/client.key\""
        "}";

    TEST_ASSERT_TRUE(ConfigLoader::parseFromJson(json, cfg));
    TEST_ASSERT_EQUAL_STRING("admin",             cfg.mqtt_username);
    TEST_ASSERT_EQUAL_STRING("secret",            cfg.mqtt_password);
    TEST_ASSERT_EQUAL_STRING("/certs/ca.crt",     cfg.ca_cert_path);
    TEST_ASSERT_EQUAL_STRING("/certs/client.crt", cfg.client_cert_path);
    TEST_ASSERT_EQUAL_STRING("/certs/client.key", cfg.client_key_path);
}

void test_parse_fails_missing_mqtt_username() {
    const char* json = "{\"mqtt_password\":\"secret\"}";
    TEST_ASSERT_FALSE(ConfigLoader::parseFromJson(json, cfg));
}

void test_parse_fails_null_input() {
    TEST_ASSERT_FALSE(ConfigLoader::parseFromJson(nullptr, cfg));
}

void test_parse_fails_malformed_json() {
    TEST_ASSERT_FALSE(ConfigLoader::parseFromJson("{not json}", cfg));
}

void test_parse_fails_empty_string() {
    TEST_ASSERT_FALSE(ConfigLoader::parseFromJson("", cfg));
}

void test_optional_fields_default_to_empty() {
    const char* json = "{\"mqtt_username\":\"u\"}";
    TEST_ASSERT_TRUE(ConfigLoader::parseFromJson(json, cfg));
    TEST_ASSERT_EQUAL_STRING("", cfg.mqtt_password);
    TEST_ASSERT_EQUAL_STRING("", cfg.ca_cert_path);
    TEST_ASSERT_EQUAL_STRING("", cfg.client_cert_path);
    TEST_ASSERT_EQUAL_STRING("", cfg.client_key_path);
}

void test_strings_are_null_terminated() {
    const char* json = "{\"mqtt_username\":\"user\",\"mqtt_password\":\"pass\"}";
    TEST_ASSERT_TRUE(ConfigLoader::parseFromJson(json, cfg));
    // Verify null terminator is within bounds
    TEST_ASSERT_EQUAL('\0', cfg.mqtt_username[sizeof(cfg.mqtt_username) - 1]);
    TEST_ASSERT_EQUAL('\0', cfg.mqtt_password[sizeof(cfg.mqtt_password) - 1]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_minimal_valid_json);
    RUN_TEST(test_parse_full_json);
    RUN_TEST(test_parse_fails_missing_mqtt_username);
    RUN_TEST(test_parse_fails_null_input);
    RUN_TEST(test_parse_fails_malformed_json);
    RUN_TEST(test_parse_fails_empty_string);
    RUN_TEST(test_optional_fields_default_to_empty);
    RUN_TEST(test_strings_are_null_terminated);
    return UNITY_END();
}
