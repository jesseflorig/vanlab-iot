#pragma once
#include <stdint.h>

/**
 * BundleEvent — typed event published between modules within a bundle.
 *
 * type:  uint8_t event type ID, defined as an enum inside each bundle.
 * value: optional scalar payload (e.g. brightness level 0–255).
 * data:  optional pointer payload; caller owns the pointed-to memory.
 */
struct BundleEvent {
    uint8_t     type;
    int32_t     value;
    const void* data;
};

using BundleEventHandler = void (*)(const BundleEvent&, void* context);

/**
 * BundleEventBus — lightweight typed event bus owned by a bundle.
 *
 * Modules within a bundle publish events and the bundle dispatches them in
 * loop(). No heap allocation occurs after subscribe() calls in setup().
 * Dispatch is synchronous and O(n) per event type.
 *
 * Usage:
 *   // In bundle setup():
 *   _bus.subscribe(Event::SwitchPressed, onSwitchPressed, this);
 *
 *   // In module loop() (via injected bus pointer):
 *   _bus->publish({Event::SwitchPressed, 0, nullptr});
 *
 *   // In bundle loop():
 *   _bus.dispatch();
 */
class BundleEventBus {
public:
    static constexpr uint8_t MAX_SUBSCRIBERS = 8;
    static constexpr uint8_t MAX_EVENT_TYPES = 16;
    static constexpr uint8_t MAX_QUEUED      = 8;

    BundleEventBus();

    /**
     * Register a handler for an event type. Call during setup() only.
     * Returns false if the subscriber table is full for this event type.
     */
    bool subscribe(uint8_t eventType, BundleEventHandler handler, void* context);

    /**
     * Enqueue an event for delivery. Safe to call from within loop().
     * Events are delivered during the next dispatch() call.
     * If the queue is full, the oldest event is dropped.
     */
    void publish(const BundleEvent& event);

    /**
     * Flush the event queue, delivering each event to all registered handlers.
     * Call once per bundle loop() iteration, after all module loop() calls.
     */
    void dispatch();

    // For testing
    uint8_t queuedCount() const { return _queueSize; }
    void    reset();

private:
    struct Subscriber {
        BundleEventHandler handler;
        void*              context;
    };

    struct SubscriberList {
        Subscriber entries[MAX_SUBSCRIBERS];
        uint8_t    count;
    };

    SubscriberList _subscribers[MAX_EVENT_TYPES];
    BundleEvent    _queue[MAX_QUEUED];
    uint8_t        _queueSize;
};
