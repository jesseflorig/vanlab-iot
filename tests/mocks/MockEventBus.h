#pragma once
#include "../../src/bundles/base/BundleEventBus.h"

/**
 * MockEventBus — BundleEventBus test double.
 *
 * Records all published events for assertion without dispatching them.
 * Use when you want to verify that a module publishes the right events
 * without running the full dispatch cycle.
 *
 * For testing the dispatch cycle itself, use the real BundleEventBus.
 */
class MockEventBus : public BundleEventBus {
public:
    static constexpr uint8_t MAX_RECORDED = 16;

    void publish(const BundleEvent& event) {
        if (_recordedCount < MAX_RECORDED)
            _recorded[_recordedCount++] = event;
    }

    // Intentionally does NOT call dispatch — use real bus for that
    void dispatch() {}

    uint8_t           publishedCount()    const { return _recordedCount; }
    const BundleEvent& publishedEvent(int i) const { return _recorded[i]; }

    void reset() {
        _recordedCount = 0;
        BundleEventBus::reset();
    }

private:
    BundleEvent _recorded[MAX_RECORDED] = {};
    uint8_t     _recordedCount          = 0;
};
