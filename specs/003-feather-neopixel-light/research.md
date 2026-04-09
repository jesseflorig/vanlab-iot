# Research: Feather NeoPixel RGB Light

## Decision 1: MQTT command dispatch for standalone IModule

**Question**: How does a standalone `IModule` subscribe to and receive MQTT messages? `IModule::loop()` does not receive `MQTTClientWrapper`. `BundleRegistry::resubscribeAll()` only iterates bundles, not standalone modules.

**Decision**: Add `virtual void resubscribe(MQTTClientWrapper& mqtt) {}` to `IModule` as a default no-op. Extend `BundleRegistry::resubscribeAll()` to also call `resubscribe()` on all registered standalone modules. `NeoPixelLightModule::resubscribe()` overrides this to call `mqtt.subscribe(commandTopic)`. In `main.cpp`, set the MQTT message callback to route incoming messages to `neoPixelLight.handleCommand()`.

**Rationale**: This is the minimal additive change that closes the existing framework gap without restructuring. All existing modules that don't override `resubscribe()` are unaffected. The MQTT callback routing in `main.cpp` is consistent with how other command dispatch would work in a single-module device config.

**Alternatives considered**:
- Promote `NeoPixelLightModule` to `IBundle`: Rejected. Would require `BundleConfig`-based YAML key and a bundle wrapper with no real sub-modules, adding unnecessary indirection.
- Global MQTT callback set once on `mqtt` in `BundleRegistry`: Rejected. The registry doesn't own the callback slot; only one callback can be set, and `main.cpp` is the right place to compose device-specific routing.
- Pass `MQTTClientWrapper&` to `IModule::loop()`: Rejected. Breaking change to all existing modules; subscription state would need to be managed carefully to avoid redundant subscribe calls every loop tick.

---

## Decision 2: HA Discovery message composition

**Question**: Where and when is the HA Discovery message published?

**Decision**: `NeoPixelLightModule::resubscribe()` (called on every MQTT reconnect by `BundleRegistry::resubscribeAll()`) publishes the discovery message after subscribing to the command topic. This satisfies FR-005 (republish on reconnect) without requiring a separate discovery timer or lifecycle hook.

**Rationale**: `resubscribe()` is already called on reconnect — piggy-backing discovery publication here is the least-overhead approach and mirrors what `LightBundle` does in the existing framework.

**Alternatives considered**:
- Publish discovery in `setup()`: Rejected. `setup()` runs before MQTT is connected; the message would be dropped.
- Separate `onConnected()` lifecycle hook on `IModule`: Rejected. Overkill — `resubscribe()` already serves this purpose.

---

## Decision 3: Brightness model

**Question**: How is brightness applied when the NeoPixel driver only accepts raw R/G/B?

**Decision**: Scale R/G/B values uniformly by `brightness / 255.0` before calling `_driver.setColor()`. Store the raw commanded color and brightness separately; recompute scaled values each time either changes.

**Rationale**: Matches the assumption documented in the spec. Simple integer arithmetic; no floating point required (use `(channel * brightness + 127) / 255` for integer rounding).

---

## Decision 4: ON command without color

**Question**: If HA sends `{"state":"ON"}` with no `color` or `brightness` field, what color should the NeoPixel show?

**Decision**: Restore the last commanded color and brightness. If no prior color was commanded since boot, use a white default (R=255, G=255, B=255) at full brightness. This matches US2, acceptance scenario 3.

**Rationale**: Retaining last-used color is the expected HA light behavior. A visible default (white) on first-boot ON command is better UX than showing nothing.

---

## Decision 5: Out-of-range RGB values

**Question**: What should happen if HA sends an R/G/B value outside 0–255 (e.g., malformed JSON)?

**Decision**: Clamp each channel to [0, 255] after parsing. Log a warning via `Serial.printf` if clamping occurs. Do not reject the command.

**Rationale**: ArduinoJson parses JSON integers natively; out-of-range values are an edge case from malformed clients. Clamping is safe and keeps the device responsive rather than silently dropping the command.

---

## Decision 6: Brightness-only command while light is OFF

**Question**: If HA sends `{"brightness": 128}` while the light is OFF, what happens?

**Decision**: Store the brightness value but do not turn the NeoPixel on. The NeoPixel remains off. When the next ON command arrives, use the stored brightness.

**Rationale**: Standard HA light behavior — adjusting brightness while off is a pre-set for the next ON event.
