#pragma once

class IGPIODriver;
class MQTTClientWrapper;

/**
 * IBundle — use-case composition layer of the vanlab-iot framework.
 *
 * A bundle groups related modules under a named use-case, owns a BundleEventBus
 * for offline inter-module coordination, and is responsible for publishing all
 * HA Discovery payloads for its entities.
 *
 * See contracts/bundle-interface.md for the full contract.
 */
class IBundle {
public:
    virtual ~IBundle() = default;

    // Called once at device init. Sets up all owned modules + event subscriptions.
    // Returns false if setup failed (bundle skipped in loop).
    virtual bool setup(IGPIODriver& gpio) = 0;

    // Called every main loop iteration. Dispatches events, calls module loops.
    // MUST be non-blocking.
    virtual void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) = 0;

    // Publish HA Discovery payloads for all entities this bundle owns.
    // Called after every MQTT reconnection. All payloads MUST use retain=true.
    virtual bool publishDiscovery(MQTTClientWrapper& mqtt) = 0;

    // Re-subscribe to all MQTT command topics after reconnection.
    virtual void resubscribe(MQTTClientWrapper& mqtt) = 0;

    virtual const char* getId() const               = 0;
    virtual const char* getAvailabilityTopic() const = 0;
};
