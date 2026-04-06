#pragma once
#include "../../../include/IBundle.h"
#include "../base/BundleEventBus.h"
#include "../modules/dimmer/DimmerModule.h"
#include "../modules/physical_switch/PhysicalSwitchModule.h"
#include "../../config/ConfigTypes.h"
#include <stdint.h>

class MQTTClientWrapper;
class IGPIODriver;

/**
 * LightBundle — dimmable light use-case profile.
 *
 * Composes a DimmerModule (PWM output) and a PhysicalSwitchModule (debounced
 * input) over a BundleEventBus. When the switch fires EVENT_SWITCH_PRESSED,
 * the bundle toggles the dimmer.
 *
 * HA Discovery: publishes a single "light" entity with brightness support.
 * MQTT command topic receives 0–255 brightness values ("0" → off, "1–255" →
 * on at that level).
 */
class LightBundle : public IBundle {
public:
    LightBundle(const BundleConfig& cfg, const DeviceConfig& dev);

    bool setup(IGPIODriver& gpio) override;
    void loop(IGPIODriver& gpio, MQTTClientWrapper& mqtt) override;
    bool publishDiscovery(MQTTClientWrapper& mqtt) override;
    void resubscribe(MQTTClientWrapper& mqtt) override;

    const char* getId() const               override { return _id; }
    const char* getAvailabilityTopic() const override { return _availTopic; }

    // Called by BundleRegistry after MQTT message arrives on the command topic.
    void handleCommand(const char* payload, unsigned int len);

private:
    // Event handler registered with the event bus (static trampoline + context)
    static void onSwitchPressed(const BundleEvent& event, void* context);

    const char*     _id;
    const char*     _deviceId;
    const char*     _deviceName;

    DimmerModule        _dimmer;
    PhysicalSwitchModule _switch;
    BundleEventBus      _bus;

    char _availTopic[128];
    char _stateTopic[128];
    char _cmdTopic[128];
};
