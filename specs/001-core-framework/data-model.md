# Data Model: 001-core-framework

## Runtime Hierarchy

```
Device
├── Bundle (named use-case profile, runtime entity)
│   ├── BundleEventBus    (intra-bundle typed event bus)
│   ├── Module A          (e.g. DimmerModule)
│   ├── Module B          (e.g. NightModeModule)
│   └── Module C          (e.g. PhysicalSwitchModule)
├── Bundle ...
└── Standalone Module     (no bundle; registered directly with Orchestrator)
```

Bundles and standalone modules are managed by the `Orchestrator` in the same loop.

---

## Entities

### DeviceConfig

Top-level device configuration struct. Produced by the YAML→C++ codegen script.
All fields are `const` — set at build time, immutable at runtime.

```cpp
struct DeviceConfig {
    const char*  device_id;       // Unique identifier; MQTT client ID + HA device key
    const char*  device_name;     // Human-readable name for HA device registry
    const char*  board;           // Board identifier (informational)
    MQTTConfig   mqtt;
    BundleConfig bundles[MAX_BUNDLES];
    uint8_t      bundle_count;
    ModuleConfig standalone_modules[MAX_MODULES];
    uint8_t      standalone_module_count;
};
```

---

### BundleConfig

```cpp
struct BundleConfig {
    const char*  type;            // Registered bundle type, e.g. "light", "environment"
    const char*  id;              // Unique within device; used in MQTT topics
    const char*  name;            // Human-readable; used in HA entity names
    const char*  topic_prefix;    // Default: "<mqtt.topic_root>/<bundle.id>"
    ModuleConfig modules[MAX_MODULES_PER_BUNDLE];
    uint8_t      module_count;
};
```

---

### ModuleConfig

```cpp
struct ModuleConfig {
    const char*  type;            // Registered module type, e.g. "dimmer", "relay"
    const char*  id;              // Unique within bundle (or device if standalone)
    const char*  name;            // Human-readable; used in HA entity name if standalone
    const char*  topic_prefix;    // Inherits from bundle unless overridden
    uint32_t     poll_interval_ms;
    GpioConfig   gpio;
    const char*  params[MAX_MODULE_PARAMS][2];
    uint8_t      param_count;
};
```

---

### GpioConfig

```cpp
struct GpioConfig {
    int8_t      pins[MAX_GPIO_PER_MODULE];   // -1 = not assigned
    const char* roles[MAX_GPIO_PER_MODULE];  // e.g. "control", "feedback", "data"
    uint8_t     count;
};
```

---

### MQTTConfig

```cpp
struct MQTTConfig {
    const char* broker_host;
    uint16_t    broker_port;      // Default: 8883
    const char* client_id;        // Defaults to device_id if null
    uint16_t    keepalive_s;      // Default: 60
    uint16_t    socket_timeout_s; // Default: 15
    const char* topic_root;       // Default: "vanlab/<device_id>"
};
```

---

### RuntimeConfig

Loaded from `/config/runtime.json` on LittleFS. Contains secrets not compiled
into firmware.

```cpp
struct RuntimeConfig {
    char mqtt_username[64];
    char mqtt_password[64];
    char ca_cert_path[64];
    char client_cert_path[64];   // Optional — mTLS
    char client_key_path[64];    // Optional — mTLS
};
```

---

### IBundle (Abstract Interface)

Pure virtual base class for all bundle types. See `contracts/bundle-interface.md`
for the full contract.

```cpp
class IBundle {
public:
    virtual ~IBundle() = default;

    // Called once at device init. Sets up all owned modules + event subscriptions.
    virtual bool setup(IGPIODriver& gpio) = 0;

    // Called every main loop iteration. Dispatches pending events, calls module loops.
    // MUST be non-blocking.
    virtual void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) = 0;

    // Publish HA Discovery payloads for all entities this bundle owns.
    // Called after every MQTT reconnection. All payloads use retain=true.
    virtual bool publishDiscovery(MQTTClientWrapper& mqtt) = 0;

    // Re-subscribe to all MQTT command topics after reconnection.
    virtual void resubscribe(MQTTClientWrapper& mqtt) = 0;

    virtual const char* getId() const = 0;
    virtual const char* getAvailabilityTopic() const = 0;
};
```

---

### IModule (Abstract Interface)

Pure virtual base class for all module implementations. Modules within a bundle
coordinate through `BundleEventBus`, not direct references.
See `contracts/module-interface.md` for the full contract.

```cpp
class IModule {
public:
    virtual ~IModule() = default;

    virtual bool        setup(IGPIODriver& gpio) = 0;
    virtual void        loop(IGPIODriver& gpio)  = 0;

    // Standalone modules publish their own discovery. Bundle-owned modules do not —
    // the bundle owns all discovery for its entities.
    virtual bool        publishDiscovery(MQTTClientWrapper& mqtt) = 0;
    virtual bool        publishState(MQTTClientWrapper& mqtt)     = 0;

    virtual const char* getId() const               = 0;
    virtual const char* getAvailabilityTopic() const = 0;
};
```

---

### BundleEventBus

Lightweight typed event bus owned by a bundle. Modules within the bundle publish
and subscribe to events through it — no direct references between sibling modules.
Dispatch is synchronous and O(n) per event type. No heap allocation after init.

```cpp
struct BundleEvent {
    uint8_t     type;    // Event type ID — defined as enum per bundle
    int32_t     value;   // Optional scalar payload (e.g. brightness level 0-255)
    const void* data;    // Optional pointer payload (null if unused)
};

using EventHandler = void(*)(const BundleEvent&, void* context);

class BundleEventBus {
public:
    // Subscribe handler to an event type. Called during setup() — not in loop().
    bool subscribe(uint8_t eventType, EventHandler handler, void* context);

    // Publish an event. Dispatches synchronously to all subscribers.
    // Safe to call from within loop() — never blocks.
    void publish(const BundleEvent& event);

    static constexpr uint8_t MAX_SUBSCRIBERS = 8;
    static constexpr uint8_t MAX_EVENT_TYPES = 16;
};
```

---

### MQTTClientWrapper

Owns the PubSubClient + ESP_SSLClient stack. Manages connection lifecycle, reconnect
FSM, and offline `StateBuffer`.

```cpp
class MQTTClientWrapper {
public:
    explicit MQTTClientWrapper(const DeviceConfig& cfg, const RuntimeConfig& rt);

    bool  connect();
    bool  isConnected() const;
    void  loop();
    bool  publish(const char* topic, const char* payload, bool retain = false);
    bool  subscribe(const char* topic);
    void  setMessageCallback(MQTT_CALLBACK_SIGNATURE);

private:
    enum class ConnState { Disconnected, Connecting, Connected, Backoff };
    ConnState    _state;
    uint32_t     _backoff_ms;
    uint32_t     _last_attempt_ms;
    StateBuffer  _buffer;
};
```

---

### StateBuffer

Ring buffer for outbound MQTT messages while the broker is unreachable.
Drained FIFO on reconnection. Fixed capacity — oldest entries dropped when full.

```cpp
class StateBuffer {
public:
    bool   push(const char* topic, const char* payload, bool retain);
    bool   pop(BufferedMessage& out);
    bool   isEmpty() const;
    size_t size() const;
    static constexpr size_t MAX_SIZE = 16;
};
```

---

### BundleRegistry

Owns all `IBundle` instances and standalone `IModule` instances. Called by the
`Orchestrator` each loop tick.

```cpp
class BundleRegistry {
public:
    void registerBundle(IBundle* bundle);
    void registerModule(IModule* module);   // Standalone modules

    void setup(IGPIODriver& gpio);
    void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt);
    void publishAllDiscovery(MQTTClientWrapper& mqtt);
    void resubscribeAll(MQTTClientWrapper& mqtt);
};
```

---

### HADiscoveryPayload

Builder for HA MQTT Discovery payloads. Used by bundles and standalone modules.
Each call to `getTopic()` / `getPayload()` returns the fully formed strings.

```cpp
class HADiscoveryPayload {
public:
    HADiscoveryPayload(const char* component,
                       const char* device_id,
                       const char* object_id);

    HADiscoveryPayload& setName(const char* name);
    HADiscoveryPayload& setStateTopic(const char* topic);
    HADiscoveryPayload& setCommandTopic(const char* topic);
    HADiscoveryPayload& setAvailabilityTopic(const char* topic);
    HADiscoveryPayload& setUnitOfMeasurement(const char* unit);
    HADiscoveryPayload& setDeviceClass(const char* cls);
    HADiscoveryPayload& setValueTemplate(const char* tmpl);
    HADiscoveryPayload& setDeviceInfo(const char* device_id,
                                      const char* device_name,
                                      const char* model);

    const char* getTopic() const;    // homeassistant/<component>/<device_id>/<object_id>/config
    const char* getPayload() const;
    bool        isValid() const;
};
```

---

### IGPIODriver (Abstract Interface)

See `contracts/hal-interface.md` for the full contract.

```cpp
class IGPIODriver {
public:
    virtual ~IGPIODriver() = default;
    virtual void     pinMode(uint8_t pin, uint8_t mode)     = 0;
    virtual void     digitalWrite(uint8_t pin, uint8_t val) = 0;
    virtual int      digitalRead(uint8_t pin)               = 0;
    virtual int      analogRead(uint8_t pin)                = 0;
    virtual void     analogWrite(uint8_t pin, int val)      = 0;
    virtual uint32_t millis()                               = 0;
    virtual void     delay(uint32_t ms)                     = 0;
};
```

---

## State Transitions

### MQTTClientWrapper Reconnect FSM

```
[Disconnected] ──connect()──▶ [Connecting]
                                   │
                           success │ failure
                                   ▼         ▼
                              [Connected]  [Backoff]
                                   │          │
                           dropped │          │ backoff_ms elapsed
                                   ▼          │
                              [Backoff] ◀─────┘
                                   │
                           retry ──┘
```

On entering `[Connected]`:
1. Drain `StateBuffer` in FIFO order
2. `BundleRegistry::publishAllDiscovery()` — all bundles + standalone modules
3. `BundleRegistry::resubscribeAll()` — all command topics

Backoff schedule: 1s → 2s → 4s → 8s → 16s → 30s (capped). Reset on connect.

### LightBundle Event Flow (example)

```
PhysicalSwitchModule
  │  publishes SwitchPressed event to BundleEventBus
  ▼
BundleEventBus
  │  dispatches to all subscribers of SwitchPressed
  ▼
LightBundle::onSwitchPressed()
  │  calls DimmerModule::toggle()
  │  calls NightModeModule::disable()
  ▼
DimmerModule / NightModeModule
     publish updated state to MQTT (or buffer if offline)
```

This coordination happens entirely without MQTT — the broker being offline does not
affect the physical switch → dimmer response path.
