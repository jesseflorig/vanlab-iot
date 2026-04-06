#include "BundleEventBus.h"
#include <string.h>

BundleEventBus::BundleEventBus() : _queueSize(0) {
    for (int i = 0; i < MAX_EVENT_TYPES; i++) {
        _subscribers[i].count = 0;
    }
}

bool BundleEventBus::subscribe(uint8_t eventType, BundleEventHandler handler, void* context) {
    if (eventType >= MAX_EVENT_TYPES) return false;
    SubscriberList& list = _subscribers[eventType];
    if (list.count >= MAX_SUBSCRIBERS) return false;
    list.entries[list.count++] = {handler, context};
    return true;
}

void BundleEventBus::publish(const BundleEvent& event) {
    if (_queueSize >= MAX_QUEUED) {
        // Drop oldest — shift queue left
        for (int i = 0; i < MAX_QUEUED - 1; i++) {
            _queue[i] = _queue[i + 1];
        }
        _queueSize = MAX_QUEUED - 1;
    }
    _queue[_queueSize++] = event;
}

void BundleEventBus::dispatch() {
    uint8_t count = _queueSize;
    _queueSize = 0;  // Clear before dispatching (handlers may publish new events)

    for (int i = 0; i < count; i++) {
        const BundleEvent& evt = _queue[i];
        if (evt.type >= MAX_EVENT_TYPES) continue;
        const SubscriberList& list = _subscribers[evt.type];
        for (int j = 0; j < list.count; j++) {
            list.entries[j].handler(evt, list.entries[j].context);
        }
    }
}

void BundleEventBus::reset() {
    _queueSize = 0;
    for (int i = 0; i < MAX_EVENT_TYPES; i++) {
        _subscribers[i].count = 0;
    }
}
