#include <unity.h>
#include "../../src/mqtt/StateBuffer.h"
#include <string.h>

static StateBuffer buf;

void setUp() {
    buf = StateBuffer();
}

void tearDown() {}

void test_initially_empty() {
    TEST_ASSERT_TRUE(buf.isEmpty());
    TEST_ASSERT_EQUAL(0, buf.size());
}

void test_push_pop_single_message() {
    TEST_ASSERT_TRUE(buf.push("a/topic", "hello", false, 1000));
    TEST_ASSERT_FALSE(buf.isEmpty());
    TEST_ASSERT_EQUAL(1, buf.size());

    BufferedMessage msg;
    TEST_ASSERT_TRUE(buf.pop(msg));
    TEST_ASSERT_EQUAL_STRING("a/topic", msg.topic);
    TEST_ASSERT_EQUAL_STRING("hello",   msg.payload);
    TEST_ASSERT_FALSE(msg.retain);
    TEST_ASSERT_EQUAL_UINT32(1000, msg.queued_at_ms);

    TEST_ASSERT_TRUE(buf.isEmpty());
}

void test_pop_empty_returns_false() {
    BufferedMessage msg;
    TEST_ASSERT_FALSE(buf.pop(msg));
}

void test_fifo_ordering() {
    buf.push("t/1", "first",  false, 0);
    buf.push("t/2", "second", false, 0);
    buf.push("t/3", "third",  false, 0);

    BufferedMessage msg;
    buf.pop(msg); TEST_ASSERT_EQUAL_STRING("first",  msg.payload);
    buf.pop(msg); TEST_ASSERT_EQUAL_STRING("second", msg.payload);
    buf.pop(msg); TEST_ASSERT_EQUAL_STRING("third",  msg.payload);
}

void test_retain_flag_preserved() {
    buf.push("t/r", "val", true, 0);
    BufferedMessage msg;
    buf.pop(msg);
    TEST_ASSERT_TRUE(msg.retain);
}

void test_oldest_dropped_when_full() {
    // Fill to capacity
    for (int i = 0; i < StateBuffer::MAX_SIZE; i++) {
        char payload[8];
        // snprintf not available without stdio — use a fixed string for each slot
        buf.push("t", (i == 0 ? "oldest" : "other"), false, (uint32_t)i);
    }
    TEST_ASSERT_EQUAL(StateBuffer::MAX_SIZE, buf.size());

    // Push one more — oldest ("oldest") should be dropped
    buf.push("t", "newest", false, 100);
    TEST_ASSERT_EQUAL(StateBuffer::MAX_SIZE, buf.size());

    // First pop should NOT be "oldest"
    BufferedMessage msg;
    buf.pop(msg);
    TEST_ASSERT_EQUAL_STRING("other", msg.payload);
}

void test_push_rejects_oversized_topic() {
    char long_topic[200];
    for (int i = 0; i < 199; i++) long_topic[i] = 'x';
    long_topic[199] = '\0';
    TEST_ASSERT_FALSE(buf.push(long_topic, "val", false, 0));
}

void test_push_rejects_oversized_payload() {
    char long_payload[600];
    for (int i = 0; i < 599; i++) long_payload[i] = 'x';
    long_payload[599] = '\0';
    TEST_ASSERT_FALSE(buf.push("t", long_payload, false, 0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_initially_empty);
    RUN_TEST(test_push_pop_single_message);
    RUN_TEST(test_pop_empty_returns_false);
    RUN_TEST(test_fifo_ordering);
    RUN_TEST(test_retain_flag_preserved);
    RUN_TEST(test_oldest_dropped_when_full);
    RUN_TEST(test_push_rejects_oversized_topic);
    RUN_TEST(test_push_rejects_oversized_payload);
    return UNITY_END();
}
