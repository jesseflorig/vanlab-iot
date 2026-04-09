#pragma once

class MQTTClientWrapper;
class IGPIODriver;
class BundleEventBus;

/**
 * IModule — capability primitive of the vanlab-iot framework.
 *
 * Modules exist in two contexts:
 *   - Bundle-owned: bundle calls setup()/loop(); bundle owns HA Discovery.
 *     Bundle-owned modules MUST NOT call publishDiscovery() themselves.
 *   - Standalone: registered directly with BundleRegistry; manages own discovery.
 *
 * See contracts/module-interface.md for the full contract.
 */
class IModule {
public:
    virtual ~IModule() = default;

    // Called once at device init. MAY call delay(). Returns false on failure.
    virtual bool setup(IGPIODriver& gpio) = 0;

    // Called every main loop iteration. MUST be non-blocking. Never call delay().
    virtual void loop(IGPIODriver& gpio) = 0;

    // Standalone modules: publish HA Discovery payload (retain=true).
    // Bundle-owned modules: no-op — the bundle handles all discovery.
    virtual bool publishDiscovery(MQTTClientWrapper& mqtt) = 0;

    // Publish current state to the module's state topic.
    virtual bool publishState(MQTTClientWrapper& mqtt) = 0;

    virtual const char* getId() const               = 0;
    virtual const char* getAvailabilityTopic() const = 0;

    // Re-subscribe to MQTT command topics after reconnection.
    // Called by BundleRegistry::resubscribeAll() on every MQTT reconnect.
    // Standalone modules override this to subscribe to their command topic.
    // Bundle-owned modules: no-op (the bundle handles resubscription).
    virtual void resubscribe(MQTTClientWrapper& mqtt) { (void)mqtt; }

    // Optional bus injection for bundle-owned modules that publish events.
    // Bundle calls this during its own setup() before calling module setup().
    // Standalone modules ignore it (default no-op).
    virtual void setBus(BundleEventBus* bus) {}
};
