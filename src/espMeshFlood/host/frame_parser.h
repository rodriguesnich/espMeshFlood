#ifndef ESPMESHFLOOD_HOST_FRAME_PARSER_H
#define ESPMESHFLOOD_HOST_FRAME_PARSER_H

#include <cstdint>
#include <functional>
#include "espMeshFlood/serialization/message_serializer.h"

namespace espMeshFlood {

/**
 * @brief Callback fired when a complete, validated frame has been parsed.
 *
 * Receives a pointer to the raw protobuf payload (framing header stripped).
 * The pointer is valid only for the duration of the callback.
 */
using FrameCallback = std::function<void(const uint8_t* payload, size_t length)>;

/**
 * @brief Streaming frame parser for the EFMP wire protocol.
 *
 * Wire format:  [0xAA magic: 1 byte][length: 2 bytes LE][protobuf payload: N bytes]
 *
 * Accepts bytes from any source (serial reads, BLE write callbacks, etc.) via
 * feed().  The state machine re-synchronises automatically on corrupt/truncated
 * data by scanning for the next magic byte.
 *
 * Thread-safety: not thread-safe; protect externally if called from an ISR.
 */
class FrameParser {
public:
    static constexpr uint8_t MAGIC_BYTE = 0xAA;
    static constexpr size_t  LENGTH_SIZE = 2;
    static constexpr size_t  MAX_PAYLOAD = MessageSerializer::max_size();

    explicit FrameParser(FrameCallback callback);

    /**
     * @brief Feed one or more bytes into the parser.
     *
     * May fire the registered callback zero or more times (once per complete frame).
     */
    void feed(const uint8_t* data, size_t len);

    /** Reset state machine to WAIT_MAGIC. */
    void reset();

private:
    enum class State { WAIT_MAGIC, WAIT_LENGTH, WAIT_PAYLOAD };

    FrameCallback callback_;
    State         state_           = State::WAIT_MAGIC;
    uint8_t       rx_buffer_[MAX_PAYLOAD];
    size_t        rx_buffer_pos_   = 0;
    uint16_t      expected_length_ = 0;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_HOST_FRAME_PARSER_H
