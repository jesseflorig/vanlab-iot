# Contract: IBundle Interface

## Purpose

`IBundle` is the use-case composition layer of the vanlab-iot framework. A bundle
groups related modules under a named use-case (e.g. `light`, `environment`), owns an
event bus for offline intra-module coordination, and is responsible for publishing all
HA Discovery payloads for its entities.

The `BundleRegistry` (via the `Orchestrator`) calls these methods. Bundle authors
implement them.

## Header

```cpp
// include/IBundle.h

class IGPIODriver;
class MQTTClientWrapper;

class IBundle {
public:
    virtual ~IBundle() = default;

    // Called once at device init. Must:
    //   - Call setup() on all owned modules
    //   - Register event subscriptions on the BundleEventBus
    // Returns false if setup failed (bundle skipped in loop).
    virtual bool setup(IGPIODriver& gpio) = 0;

    // Called every main loop iteration. Must:
    //   - Dispatch pending events via the BundleEventBus
    //   - Call loop() on all owned modules
    // MUST be non-blocking.
    virtual void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) = 0;

    // Publish HA Discovery payloads for all entities this bundle owns.
    // Called after every MQTT reconnection.
    // Each payload MUST use retain=true.
    // Each payload MUST include the shared device object.
    // Returns false if any publish failed.
    virtual bool publishDiscovery(MQTTClientWrapper& mqtt) = 0;

    // Re-subscribe to all MQTT command topics for this bundle's entities.
    // Called after every MQTT reconnection.
    virtual void resubscribe(MQTTClientWrapper& mqtt) = 0;

    // Returns the bundle's unique ID (matches BundleConfig.id).
    // Must return a stable non-null value after construction.
    virtual const char* getId() const = 0;

    // Returns the device-level availability topic.
    // Convention: <mqtt.topic_root>/availability
    virtual const char* getAvailabilityTopic() const = 0;
};
```

## Rules

1. `loop()` MUST be non-blocking. Bundles must not call `delay()` in loop.

2. Bundles MUST own their `BundleEventBus` instance as a member — not shared across
   bundles. Events do not cross bundle boundaries.

3. Bundle behaviors MUST be hardcoded in C++. No runtime rule engine. Key values
   (dim levels, thresholds) may come from `BundleConfig.params` loaded at init —
   but the logic that acts on them is always compiled code.

4. `publishDiscovery()` MUST use `HADiscoveryPayload::setDeviceInfo()` with the
   device's `device_id` and `device_name` on every payload. This is what causes HA
   to group all entities under one device card.

5. Bundle constructors MUST accept `const BundleConfig&` and `const DeviceConfig&`.
   No other dependencies at construction time.

6. Modules within a bundle MUST NOT hold references to sibling modules. All
   coordination goes through the bundle's `BundleEventBus`.

7. Standalone modules (not in a bundle) follow the `IModule` contract and publish
   their own discovery. They are not `IBundle` implementations.

## LightBundle Example

```cpp
class LightBundle : public IBundle {
public:
    LightBundle(const BundleConfig& cfg, const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override {
        _dimmer.setup(gpio);
        _nightMode.setup(gpio);
        _switch.setup(gpio);

        // Register event handlers — no direct module references cross-handed
        _bus.subscribe(Event::SwitchPressed,  onSwitchPressed,  this);
        _bus.subscribe(Event::NightModeOn,    onNightModeOn,    this);
        _bus.subscribe(Event::NightModeOff,   onNightModeOff,   this);
        return true;
    }

    void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) override {
        _switch.loop(gpio);         // May publish SwitchPressed to bus
        _nightMode.loop(gpio);      // May publish NightModeOn/Off to bus
        _bus.dispatch();            // Deliver all pending events synchronously
        _dimmer.loop(gpio);         // Responds to state set during dispatch
        // Publish any state changes queued during this tick
        _dimmer.publishState(mqtt);
        _nightMode.publishState(mqtt);
    }

    bool publishDiscovery(MQTTClientWrapper& mqtt) override {
        // Bundle owns all entity discovery — modules do not call publishDiscovery()
        return _dimmer.publishEntityDiscovery(mqtt, _deviceInfo)
            && _nightMode.publishEntityDiscovery(mqtt, _deviceInfo);
    }

    void resubscribe(MQTTClientWrapper& mqtt) override {
        _dimmer.subscribe(mqtt);
    }

private:
    enum Event : uint8_t { SwitchPressed, NightModeOn, NightModeOff };

    static void onSwitchPressed(const BundleEvent& e, void* ctx) {
        auto* self = static_cast<LightBundle*>(ctx);
        self->_dimmer.toggle();
    }

    static void onNightModeOn(const BundleEvent& e, void* ctx) {
        auto* self = static_cast<LightBundle*>(ctx);
        self->_dimmer.setLevel(self->_nightDimLevel);
    }

    DimmerModule    _dimmer;
    NightModeModule _nightMode;
    PhysicalSwitchModule _switch;
    BundleEventBus  _bus;
    DeviceInfo      _deviceInfo;
    uint8_t         _nightDimLevel = 30;  // Hardcoded; overrideable via BundleConfig.params
};
```

## Testing

Bundle tests live in `tests/unit/bundles/test_<bundle_name>.cpp`.

Use `MockGPIODriver` and `MockEventBus` to verify:
- `setup()` subscribes to the correct event types
- Events dispatched by one module trigger the expected state changes in another
  (via the real `BundleEventBus` or `MockEventBus` depending on what you're testing)
- `publishDiscovery()` produces valid HA payloads for all bundle entities
- `loop()` returns without blocking (assert on call count within a tick, not on time)
