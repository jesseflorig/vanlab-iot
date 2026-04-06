#pragma once
#include "StateBuffer.h"
#include "../config/ConfigTypes.h"
#include <stdint.h>
#include <stddef.h>

// Forward-declare Arduino library types so this header compiles on native
#ifndef ARDUINO
struct EthernetClient {};
#endif

/**
 * MQTTClientWrapper — MQTTS connectivity with offline state buffering.
 *
 * Wraps PubSubClient + ESP_SSLClient with:
 *   - ConnState FSM (Disconnected → Connecting → Connected → Backoff)
 *   - Exponential backoff: 1s → 2s → 4s → 8s → 16s → 30s (capped)
 *   - StateBuffer drain on reconnection
 *   - Reconnect callback for BundleRegistry to re-publish discovery
 *
 * See data-model.md for FSM diagram and state transition rules.
 */
class MQTTClientWrapper {
public:
    enum class ConnState { Disconnected, Connecting, Connected, Backoff };

    using ReconnectCallback = void (*)(void* context);

    MQTTClientWrapper(const DeviceConfig& cfg, const RuntimeConfig& rt);

    // Call once in setup() to initialize network + MQTT client
    void begin();

    // Call every loop iteration — drives FSM, drains buffer on reconnect
    void loop();

    bool isConnected() const { return _state == ConnState::Connected; }

    // Returns true if the state transitioned TO Connected this tick.
    // Callers use this to trigger re-discovery and re-subscribe.
    bool justReconnected() const { return _justReconnected; }

    ConnState state() const { return _state; }

    // Publish a message. If disconnected, buffers it in StateBuffer.
    bool publish(const char* topic, const char* payload, bool retain = false);

    bool subscribe(const char* topic);

    // Register a callback invoked immediately after reconnection,
    // before the StateBuffer is drained.
    void onReconnect(ReconnectCallback cb, void* context);

    void setMessageCallback(void (*callback)(const char*, uint8_t*, unsigned int));

    // Expose backoff state for testing
    uint32_t currentBackoffMs() const { return _backoffMs; }

private:
    bool attemptConnect();
    void onConnected();
    void drainBuffer();

    const DeviceConfig&  _cfg;
    const RuntimeConfig& _rt;

    ConnState _state;
    uint32_t  _backoffMs;
    uint32_t  _lastAttemptMs;
    bool      _justReconnected;

    StateBuffer _buffer;

    ReconnectCallback _reconnectCb;
    void*             _reconnectCtx;

    static constexpr uint32_t BACKOFF_INITIAL_MS = 1000;
    static constexpr uint32_t BACKOFF_MAX_MS     = 30000;
};
