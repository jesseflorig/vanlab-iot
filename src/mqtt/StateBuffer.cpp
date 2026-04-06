#include "StateBuffer.h"

StateBuffer::StateBuffer() : _head(0), _size(0) {}

bool StateBuffer::push(const char* topic, const char* payload, bool retain, uint32_t now_ms) {
    if (!topic || !payload) return false;
    if (strlen(topic) >= sizeof(BufferedMessage::topic))     return false;
    if (strlen(payload) >= sizeof(BufferedMessage::payload)) return false;

    if (_size == MAX_SIZE) {
        // Drop oldest — advance head
        _head = (_head + 1) % MAX_SIZE;
        _size--;
    }

    uint8_t tail = (_head + _size) % MAX_SIZE;
    strncpy(_buf[tail].topic,   topic,   sizeof(_buf[tail].topic)   - 1);
    strncpy(_buf[tail].payload, payload, sizeof(_buf[tail].payload) - 1);
    _buf[tail].topic[sizeof(_buf[tail].topic)   - 1] = '\0';
    _buf[tail].payload[sizeof(_buf[tail].payload) - 1] = '\0';
    _buf[tail].retain       = retain;
    _buf[tail].queued_at_ms = now_ms;
    _size++;
    return true;
}

bool StateBuffer::pop(BufferedMessage& out) {
    if (_size == 0) return false;
    out   = _buf[_head];
    _head = (_head + 1) % MAX_SIZE;
    _size--;
    return true;
}
