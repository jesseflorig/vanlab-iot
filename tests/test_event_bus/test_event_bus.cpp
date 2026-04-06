#include <unity.h>
#include "../../src/bundles/base/BundleEventBus.h"
#include "../mocks/MockEventBus.h"

// ── Tracking state for handler callbacks ─────────────────────────────────────

static uint8_t  s_lastType  = 255;
static int32_t  s_lastValue = -1;
static int      s_callCount = 0;
static void*    s_lastCtx   = nullptr;

static void testHandler(const BundleEvent& ev, void* ctx) {
    s_lastType  = ev.type;
    s_lastValue = ev.value;
    s_lastCtx   = ctx;
    s_callCount++;
}

void setUp() {
    s_lastType  = 255;
    s_lastValue = -1;
    s_callCount = 0;
    s_lastCtx   = nullptr;
}

void tearDown() {}

// ── Real BundleEventBus tests ─────────────────────────────────────────────────

void test_subscribe_and_dispatch_basic() {
    BundleEventBus bus;
    int ctx = 42;
    bus.subscribe(0, testHandler, &ctx);

    bus.publish({0, 7, nullptr});
    bus.dispatch();

    TEST_ASSERT_EQUAL(1, s_callCount);
    TEST_ASSERT_EQUAL(0, s_lastType);
    TEST_ASSERT_EQUAL(7, s_lastValue);
    TEST_ASSERT_EQUAL_PTR(&ctx, s_lastCtx);
}

void test_dispatch_clears_queue() {
    BundleEventBus bus;
    bus.subscribe(1, testHandler, nullptr);

    bus.publish({1, 0, nullptr});
    bus.dispatch();
    TEST_ASSERT_EQUAL(1, s_callCount);

    // Second dispatch — queue already drained, no additional calls
    bus.dispatch();
    TEST_ASSERT_EQUAL(1, s_callCount);
}

void test_unsubscribed_event_not_delivered() {
    BundleEventBus bus;
    bus.subscribe(0, testHandler, nullptr);

    bus.publish({5, 0, nullptr}); // type 5 has no subscriber
    bus.dispatch();

    TEST_ASSERT_EQUAL(0, s_callCount);
}

void test_multiple_events_dispatched_fifo() {
    BundleEventBus bus;
    bus.subscribe(0, testHandler, nullptr);

    bus.publish({0, 1, nullptr});
    bus.publish({0, 2, nullptr});
    bus.publish({0, 3, nullptr});
    TEST_ASSERT_EQUAL(3, bus.queuedCount());

    bus.dispatch();
    // testHandler overwrites on each call; just verify it was called 3 times
    TEST_ASSERT_EQUAL(3, s_callCount);
    TEST_ASSERT_EQUAL(3, s_lastValue); // last event value
}

void test_queue_drops_oldest_when_full() {
    BundleEventBus bus;
    // Fill the queue beyond MAX_QUEUED
    for (int i = 0; i < BundleEventBus::MAX_QUEUED + 2; i++) {
        bus.publish({0, (int32_t)i, nullptr});
    }
    // Queue size should be capped
    TEST_ASSERT_EQUAL(BundleEventBus::MAX_QUEUED, bus.queuedCount());
}

void test_reset_clears_queue() {
    BundleEventBus bus;
    bus.publish({0, 1, nullptr});
    bus.reset();
    TEST_ASSERT_EQUAL(0, bus.queuedCount());
}

// ── MockEventBus tests ────────────────────────────────────────────────────────

void test_mock_bus_records_published_events() {
    MockEventBus mock;
    mock.publish({2, 42, nullptr});
    mock.publish({3, -1, nullptr});

    TEST_ASSERT_EQUAL(2, mock.publishedCount());
    TEST_ASSERT_EQUAL(2,  mock.publishedEvent(0).type);
    TEST_ASSERT_EQUAL(42, mock.publishedEvent(0).value);
    TEST_ASSERT_EQUAL(3,  mock.publishedEvent(1).type);
}

void test_mock_bus_dispatch_is_noop() {
    MockEventBus mock;
    mock.subscribe(0, testHandler, nullptr);
    mock.publish({0, 99, nullptr});
    mock.dispatch(); // should NOT call handler

    TEST_ASSERT_EQUAL(0, s_callCount);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_subscribe_and_dispatch_basic);
    RUN_TEST(test_dispatch_clears_queue);
    RUN_TEST(test_unsubscribed_event_not_delivered);
    RUN_TEST(test_multiple_events_dispatched_fifo);
    RUN_TEST(test_queue_drops_oldest_when_full);
    RUN_TEST(test_reset_clears_queue);
    RUN_TEST(test_mock_bus_records_published_events);
    RUN_TEST(test_mock_bus_dispatch_is_noop);
    return UNITY_END();
}
