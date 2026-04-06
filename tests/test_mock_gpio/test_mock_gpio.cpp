#include <unity.h>
#include "../mocks/MockGPIODriver.h"

static MockGPIODriver gpio;

void setUp() {
    gpio.reset();
}

void tearDown() {}

void test_pinMode_records_call() {
    gpio.pinMode(5, 1 /*OUTPUT*/);
    TEST_ASSERT_EQUAL(1, gpio.pinModeCallCount());
    TEST_ASSERT_EQUAL(5, gpio.pinModeCall(0).pin);
    TEST_ASSERT_EQUAL(1, gpio.pinModeCall(0).mode);
}

void test_digitalWrite_records_call() {
    gpio.digitalWrite(3, 1);
    TEST_ASSERT_EQUAL(1, gpio.digitalWriteCallCount());
    TEST_ASSERT_EQUAL(3, gpio.digitalWriteCall(0).pin);
    TEST_ASSERT_EQUAL(1, gpio.digitalWriteCall(0).val);
}

void test_digitalRead_returns_injected_value() {
    gpio.setDigitalRead(7, 1);
    TEST_ASSERT_EQUAL(1, gpio.digitalRead(7));
    TEST_ASSERT_EQUAL(0, gpio.digitalRead(0));  // default is 0
}

void test_analogWrite_records_call() {
    gpio.analogWrite(9, 128);
    TEST_ASSERT_EQUAL(1, gpio.analogWriteCallCount());
    TEST_ASSERT_EQUAL(9,   gpio.analogWriteCall(0).pin);
    TEST_ASSERT_EQUAL(128, gpio.analogWriteCall(0).val);
}

void test_analogRead_returns_injected_value() {
    gpio.setAnalogRead(4, 512);
    TEST_ASSERT_EQUAL(512, gpio.analogRead(4));
}

void test_millis_returns_set_value() {
    gpio.setMillis(12345);
    TEST_ASSERT_EQUAL_UINT32(12345, gpio.millis());
}

void test_reset_clears_state() {
    gpio.pinMode(1, 1);
    gpio.digitalWrite(2, 1);
    gpio.analogWrite(3, 100);
    gpio.setMillis(9999);

    gpio.reset();

    TEST_ASSERT_EQUAL(0, gpio.pinModeCallCount());
    TEST_ASSERT_EQUAL(0, gpio.digitalWriteCallCount());
    TEST_ASSERT_EQUAL(0, gpio.analogWriteCallCount());
    TEST_ASSERT_EQUAL_UINT32(0, gpio.millis());
}

void test_out_of_range_pin_returns_zero() {
    TEST_ASSERT_EQUAL(0, gpio.digitalRead(MockGPIODriver::MAX_PINS));
    TEST_ASSERT_EQUAL(0, gpio.analogRead(MockGPIODriver::MAX_PINS));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pinMode_records_call);
    RUN_TEST(test_digitalWrite_records_call);
    RUN_TEST(test_digitalRead_returns_injected_value);
    RUN_TEST(test_analogWrite_records_call);
    RUN_TEST(test_analogRead_returns_injected_value);
    RUN_TEST(test_millis_returns_set_value);
    RUN_TEST(test_reset_clears_state);
    RUN_TEST(test_out_of_range_pin_returns_zero);
    return UNITY_END();
}
