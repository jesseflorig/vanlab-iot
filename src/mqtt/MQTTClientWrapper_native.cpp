/**
 * MQTTClientWrapper_native.cpp — No-op stub for native (unit test) builds.
 *
 * Provides symbol definitions required by the linker when
 * MQTTClientWrapper.cpp (which depends on PubSubClient + ESP_SSLClient)
 * is excluded via build_src_filter. All methods are no-ops or return false.
 *
 * This file is excluded from embedded builds via build_src_filter.
 */

#ifndef ARDUINO

#include "MQTTClientWrapper.h"
#include <string.h>

MQTTClientWrapper::MQTTClientWrapper(const DeviceConfig& cfg, const RuntimeConfig& rt)
    : _cfg(cfg)
    , _rt(rt)
    , _state(ConnState::Disconnected)
    , _backoffMs(BACKOFF_INITIAL_MS)
    , _lastAttemptMs(0)
    , _justReconnected(false)
    , _reconnectCb(nullptr)
    , _reconnectCtx(nullptr)
{}

void  MQTTClientWrapper::begin()                                      {}
void  MQTTClientWrapper::loop()                                       {}
bool  MQTTClientWrapper::publish(const char*, const char*, bool)      { return false; }
bool  MQTTClientWrapper::subscribe(const char*)                       { return false; }
void  MQTTClientWrapper::onReconnect(ReconnectCallback cb, void* ctx) { _reconnectCb = cb; _reconnectCtx = ctx; }
void  MQTTClientWrapper::setMessageCallback(void (*)(const char*, uint8_t*, unsigned int)) {}

bool  MQTTClientWrapper::attemptConnect()                             { return false; }
void  MQTTClientWrapper::onConnected()                                {}
void  MQTTClientWrapper::drainBuffer()                                {}

#endif // !ARDUINO
