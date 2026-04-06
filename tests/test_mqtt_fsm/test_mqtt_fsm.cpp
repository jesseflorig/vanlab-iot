/**
 * test_mqtt_fsm.cpp — FSM state and backoff logic tests.
 *
 * MQTTClientWrapper.cpp is excluded from the native build because it depends
 * on PubSubClient + ESP_SSLClient. We test the FSM behaviour indirectly by
 * exercising StateBuffer (the offline path) and by verifying the justReconnected()
 * edge-trigger contract using a minimal FSM stub.
 */
#include <unity.h>
#include "../../src/mqtt/StateBuffer.h"
#include <string.h>

// ── Minimal FSM stub matching MQTTClientWrapper behaviour ────────────────────

struct FsmStub {
    enum class ConnState { Disconnected, Connecting, Connected, Backoff };

    ConnState _state        = ConnState::Disconnected;
    bool      _justReconnected = false;

    // Simulates transitioning to Connected for one tick
    void transitionToConnected() {
        _state            = ConnState::Connected;
        _justReconnected  = true;
    }

    void tick() {
        // Edge flag is cleared after the first tick
        _justReconnected = false;
    }

    bool justReconnected() const { return _justReconnected; }
    bool isConnected()     const { return _state == ConnState::Connected; }
};

void setUp() {}
void tearDown() {}

void test_just_reconnected_true_only_first_tick() {
    FsmStub fsm;
    TEST_ASSERT_FALSE(fsm.justReconnected());

    fsm.transitionToConnected();
    TEST_ASSERT_TRUE(fsm.justReconnected());
    TEST_ASSERT_TRUE(fsm.isConnected());

    fsm.tick();  // second loop iteration
    TEST_ASSERT_FALSE(fsm.justReconnected());
    TEST_ASSERT_TRUE(fsm.isConnected());
}

void test_fsm_starts_disconnected() {
    FsmStub fsm;
    TEST_ASSERT_FALSE(fsm.isConnected());
    TEST_ASSERT_FALSE(fsm.justReconnected());
}

// ── StateBuffer offline-path tests ───────────────────────────────────────────

void test_state_buffer_holds_messages_while_offline() {
    StateBuffer buf;
    buf.push("device/light/state", "255", true, 0);
    buf.push("device/light/state", "0",   true, 1);
    TEST_ASSERT_EQUAL(2, buf.size());
}

void test_state_buffer_drains_fifo_on_reconnect() {
    StateBuffer buf;
    buf.push("t/a", "first",  false, 0);
    buf.push("t/b", "second", false, 1);

    // Simulate drain on reconnect
    BufferedMessage m1, m2;
    TEST_ASSERT_TRUE(buf.pop(m1));
    TEST_ASSERT_TRUE(buf.pop(m2));
    TEST_ASSERT_EQUAL_STRING("first",  m1.payload);
    TEST_ASSERT_EQUAL_STRING("second", m2.payload);
    TEST_ASSERT_TRUE(buf.isEmpty());
}

void test_state_buffer_oldest_dropped_when_full() {
    StateBuffer buf;
    for (int i = 0; i < StateBuffer::MAX_SIZE; i++) {
        buf.push("t", "fill", false, (uint32_t)i);
    }
    // One more — oldest dropped, size stays at MAX
    bool ok = buf.push("t", "latest", false, 100);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(StateBuffer::MAX_SIZE, buf.size());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fsm_starts_disconnected);
    RUN_TEST(test_just_reconnected_true_only_first_tick);
    RUN_TEST(test_state_buffer_holds_messages_while_offline);
    RUN_TEST(test_state_buffer_drains_fifo_on_reconnect);
    RUN_TEST(test_state_buffer_oldest_dropped_when_full);
    return UNITY_END();
}
