#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/**
 * MockMQTTClientWrapper — minimal MQTT test double for NeoPixelLightModule tests.
 *
 * Records subscribe() and publish() calls for assertion in unit tests.
 * Does not depend on MQTTClientWrapper's SSL/network internals.
 */
class MockMQTTClientWrapper {
public:
    static constexpr uint8_t MAX_CALLS = 16;

    // ── subscribe recording ───────────────────────────────────────────────────

    struct SubCall {
        char topic[128];
    };

    SubCall  subCalls[MAX_CALLS];
    uint8_t  subCount = 0;

    bool subscribe(const char* topic) {
        if (subCount < MAX_CALLS) {
            strncpy(subCalls[subCount].topic, topic, sizeof(subCalls[0].topic) - 1);
            subCalls[subCount].topic[sizeof(subCalls[0].topic) - 1] = '\0';
            subCount++;
        }
        return true;
    }

    // ── publish recording ─────────────────────────────────────────────────────

    struct PubCall {
        char topic[256];
        char payload[1024];
        bool retain;
    };

    PubCall pubCalls[MAX_CALLS];
    uint8_t pubCount = 0;

    bool publish(const char* topic, const char* payload, bool retain = false) {
        if (pubCount < MAX_CALLS) {
            strncpy(pubCalls[pubCount].topic,   topic,   sizeof(pubCalls[0].topic)   - 1);
            strncpy(pubCalls[pubCount].payload, payload, sizeof(pubCalls[0].payload) - 1);
            pubCalls[pubCount].topic[sizeof(pubCalls[0].topic) - 1]     = '\0';
            pubCalls[pubCount].payload[sizeof(pubCalls[0].payload) - 1] = '\0';
            pubCalls[pubCount].retain = retain;
            pubCount++;
        }
        return true;
    }

    // ── helpers ───────────────────────────────────────────────────────────────

    void reset() {
        subCount = 0;
        pubCount = 0;
    }

    bool wasSubscribedTo(const char* topic) const {
        for (uint8_t i = 0; i < subCount; i++) {
            if (strcmp(subCalls[i].topic, topic) == 0) return true;
        }
        return false;
    }

    bool wasPublishedTo(const char* topic) const {
        for (uint8_t i = 0; i < pubCount; i++) {
            if (strcmp(pubCalls[i].topic, topic) == 0) return true;
        }
        return false;
    }

    const char* lastPayloadFor(const char* topic) const {
        // Return the payload of the last publish to the given topic
        for (int i = (int)pubCount - 1; i >= 0; i--) {
            if (strcmp(pubCalls[i].topic, topic) == 0) return pubCalls[i].payload;
        }
        return nullptr;
    }
};
