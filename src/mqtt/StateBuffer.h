#pragma once
#include <stdint.h>
#include <string.h>

/**
 * StateBuffer — ring buffer of outbound MQTT messages for offline operation.
 *
 * Pushes messages when the broker is unreachable. Drains FIFO on reconnection.
 * When full, the oldest entry is dropped to make room for the newest.
 */
struct BufferedMessage {
    char     topic[128];
    char     payload[512];
    bool     retain;
    uint32_t queued_at_ms;
};

class StateBuffer {
public:
    static constexpr uint8_t MAX_SIZE = 16;

    StateBuffer();

    /**
     * Enqueue a message. Returns false if topic or payload exceeds buffer limits.
     * If the buffer is already full, the oldest entry is silently dropped.
     */
    bool push(const char* topic, const char* payload, bool retain, uint32_t now_ms = 0);

    /**
     * Dequeue the oldest message into `out`. Returns false if the buffer is empty.
     */
    bool pop(BufferedMessage& out);

    bool    isEmpty() const { return _size == 0; }
    uint8_t size()    const { return _size; }

private:
    BufferedMessage _buf[MAX_SIZE];
    uint8_t         _head;   // index of oldest entry
    uint8_t         _size;
};
