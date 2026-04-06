# Contract: IModule Interface

## Purpose

`IModule` is the capability primitive of the vanlab-iot framework. Every device
capability (sensor, relay, LED, button, etc.) is implemented as an `IModule`. Modules
exist in one of two contexts:

- **Bundle-owned**: the module is composed inside an `IBundle`. The bundle calls
  `setup()` and `loop()`. The bundle owns HA Discovery for all its entities —
  bundle-owned modules MUST NOT call `publishDiscovery()` on their own.
- **Standalone**: the module is registered directly with the `BundleRegistry` (via
  `Orchestrator`). It manages its own discovery and state publication.

The `BundleRegistry` (for standalone) or the owning `IBundle` calls these methods.

## Header

```cpp
// include/IModule.h

class MQTTClientWrapper;
class IGPIODriver;

class IModule {
public:
    virtual ~IModule() = default;

    // Called once during device setup. Initialize GPIO, allocate resources.
    // Returns false if setup failed (module will be skipped in loop).
    virtual bool setup(IGPIODriver& gpio) = 0;

    // Called every main loop iteration. MUST be non-blocking.
    // Long operations (I2C reads, sensor polling) MUST use elapsed-time guards.
    virtual void loop(IGPIODriver& gpio) = 0;

    // Publish the HA MQTT Discovery payload for this module.
    // Called after every MQTT reconnection. Must use retain=true.
    // Returns false if publish failed (framework will retry on next reconnect).
    virtual bool publishDiscovery(MQTTClientWrapper& mqtt) = 0;

    // Publish current state to the module's state topic.
    // Called by the module itself from loop() when state changes,
    // or by the framework after reconnection to restore broker state.
    virtual bool publishState(MQTTClientWrapper& mqtt) = 0;

    // Returns the module's unique ID (matches ModuleConfig.id).
    // Must return a stable, non-null value after construction.
    virtual const char* getId() const = 0;

    // Returns the availability topic for this module.
    // Convention: <topic_prefix>/<id>/availability
    // Framework publishes "online"/"offline" to this topic.
    virtual const char* getAvailabilityTopic() const = 0;
};
```

## Rules

1. `loop()` MUST be non-blocking. Never call `delay()`. Use `millis()` guards for
   timed operations.

2. `setup()` MAY call `delay()` — it runs once, not in the hot path.

3. `publishDiscovery()` — **standalone modules only**. Payload MUST include `unique_id`,
   `device`, `availability_topic`, and `state_topic` at minimum. Bundle-owned modules
   MUST NOT call `publishDiscovery()` — the owning bundle handles all discovery.

4. `getId()` MUST return the same value for the lifetime of the object.

5. Module constructors MUST accept `const ModuleConfig&` and `const DeviceConfig&`.
   No other dependencies at construction time.

6. Modules MUST NOT hold a reference or pointer to `IGPIODriver` or
   `MQTTClientWrapper` beyond the duration of a single method call.

7. Bundle-owned modules MUST NOT hold references to sibling modules. All
   inter-module coordination goes through the bundle's `BundleEventBus`.

## Minimal Implementation Example

```cpp
class StatusLEDModule : public IModule {
public:
    StatusLEDModule(const ModuleConfig& cfg, const DeviceConfig& dev)
        : _id(cfg.id), _pin(cfg.gpio.pins[0]) {}

    bool setup(IGPIODriver& gpio) override {
        gpio.pinMode(_pin, OUTPUT);
        gpio.digitalWrite(_pin, LOW);
        return true;
    }

    void loop(IGPIODriver& gpio) override {
        // Non-blocking blink example
        if (gpio.millis() - _last_ms > 500) {
            _state = !_state;
            gpio.digitalWrite(_pin, _state ? HIGH : LOW);
            _last_ms = gpio.millis();
        }
    }

    bool publishDiscovery(MQTTClientWrapper& mqtt) override { return true; }
    bool publishState(MQTTClientWrapper& mqtt) override     { return true; }
    const char* getId() const override              { return _id; }
    const char* getAvailabilityTopic() const override { return _avail_topic; }

private:
    const char* _id;
    uint8_t     _pin;
    bool        _state      = false;
    uint32_t    _last_ms    = 0;
    char        _avail_topic[128];
};
```

## Testing

Tests for a module implementation live in `tests/unit/modules/test_<module_name>.cpp`.
Use `MockGPIODriver` from `tests/mocks/` to inject a controllable HAL. Verify:
- `setup()` calls `pinMode()` with correct pin and mode
- `loop()` does not block (assert completion within a deadline)
- `publishDiscovery()` produces a valid HA payload (validate JSON structure)
- `publishState()` publishes to the expected topic with expected payload
