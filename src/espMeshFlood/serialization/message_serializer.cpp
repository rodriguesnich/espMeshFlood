#include "espMeshFlood/serialization/message_serializer.h"
#include "espMeshFlood/types.h"
#include <cstring>

namespace espMeshFlood {

/**
 * @brief Simple Protobuf-like serialization without external dependencies
 * 
 * This is a manual implementation that matches the nanopb format.
 * Field encoding (wire type 2 = length-delimited, wire type 0 = varint):
 * 
 * origin_node_id (field 1, wire type 0): varint
 * message_id (field 2, wire type 0): varint
 * sender_id (field 3, wire type 0): varint
 * timestamp (field 4, wire type 0): varint
 * type (field 5, wire type 0): varint
 * ttl (field 6, wire type 0): varint
 * payload (field 7, wire type 2): length-delimited
 */

static void write_varint(uint8_t*& buffer, size_t& remaining, uint64_t value) {
    while (value >= 0x80) {
        if (remaining < 1) return;
        *buffer++ = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
        remaining--;
    }
    if (remaining < 1) return;
    *buffer++ = (uint8_t)(value & 0x7F);
    remaining--;
}

static void write_field_key(uint8_t*& buffer, size_t& remaining, uint32_t field_num, uint32_t wire_type) {
    uint32_t key = (field_num << 3) | wire_type;
    write_varint(buffer, remaining, key);
}

static bool read_varint(const uint8_t*& buffer, size_t& remaining, uint64_t& value) {
    value = 0;
    int shift = 0;
    while (remaining > 0) {
        uint8_t byte = *buffer++;
        remaining--;
        value |= ((uint64_t)(byte & 0x7F) << shift);
        if ((byte & 0x80) == 0) return true;
        shift += 7;
    }
    return false;
}

static bool read_field_key(const uint8_t*& buffer, size_t& remaining, uint32_t& field_num, uint32_t& wire_type) {
    uint64_t key;
    if (!read_varint(buffer, remaining, key)) return false;
    field_num = (uint32_t)(key >> 3);
    wire_type = (uint32_t)(key & 0x07);
    return true;
}

size_t MessageSerializer::serialize(const MeshMessage& message, uint8_t* buffer, size_t buffer_size) {
    uint8_t* start = buffer;
    size_t remaining = buffer_size;

    // Field 1: origin_node_id (varint)
    write_field_key(buffer, remaining, 1, 0);
    write_varint(buffer, remaining, message.origin_node_id);

    // Field 2: message_id (varint)
    write_field_key(buffer, remaining, 2, 0);
    write_varint(buffer, remaining, message.message_id);

    // Field 3: sender_id (varint)
    write_field_key(buffer, remaining, 3, 0);
    write_varint(buffer, remaining, message.sender_id);

    // Field 4: timestamp (varint)
    write_field_key(buffer, remaining, 4, 0);
    write_varint(buffer, remaining, message.timestamp);

    // Field 5: type (varint)
    write_field_key(buffer, remaining, 5, 0);
    write_varint(buffer, remaining, static_cast<uint32_t>(message.type));

    // Field 6: ttl (varint)
    write_field_key(buffer, remaining, 6, 0);
    write_varint(buffer, remaining, message.ttl);

    // Field 7: payload (length-delimited)
    if (!message.payload.empty()) {
        write_field_key(buffer, remaining, 7, 2);
        write_varint(buffer, remaining, message.payload.size());
        if (remaining < message.payload.size()) {
            return 0;  // Error: buffer too small
        }
        std::memcpy(buffer, message.payload.data(), message.payload.size());
        buffer += message.payload.size();
        remaining -= message.payload.size();
    }

    return buffer_size - remaining;
}

bool MessageSerializer::deserialize(const uint8_t* buffer, size_t buffer_size, MeshMessage& message) {
    const uint8_t* start = buffer;
    size_t remaining = buffer_size;
    message = MeshMessage();  // Reset to defaults

    // Empty buffer is invalid
    if (buffer_size == 0) {
        return false;
    }

    while (remaining > 0) {
        uint32_t field_num, wire_type;
        if (!read_field_key(buffer, remaining, field_num, wire_type)) {
            break;  // End of message or error
        }

        uint64_t varint_value;
        switch (field_num) {
            case 1:  // origin_node_id
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.origin_node_id = static_cast<uint32_t>(varint_value);
                }
                break;
            case 2:  // message_id
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.message_id = static_cast<uint32_t>(varint_value);
                }
                break;
            case 3:  // sender_id
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.sender_id = (uint32_t)varint_value;
                }
                break;
            case 4:  // timestamp
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.timestamp = varint_value;
                }
                break;
            case 5:  // type
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.type = static_cast<MessageType>(varint_value);
                }
                break;
            case 6:  // ttl
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                    message.ttl = (uint32_t)varint_value;
                }
                break;
            case 7:  // payload (length-delimited)
                if (wire_type == 2) {
                    uint64_t length;
                    if (!read_varint(buffer, remaining, length)) return false;
                    if (remaining < (size_t)length) return false;
                    message.payload.resize((size_t)length);
                    std::memcpy(message.payload.data(), buffer, (size_t)length);
                    buffer += (size_t)length;
                    remaining -= (size_t)length;
                }
                break;
            default:
                // Unknown field, skip it (varint fields are 0, length-delimited are 2)
                if (wire_type == 0) {
                    if (!read_varint(buffer, remaining, varint_value)) return false;
                } else if (wire_type == 2) {
                    uint64_t length;
                    if (!read_varint(buffer, remaining, length)) return false;
                    if (remaining < (size_t)length) return false;
                    buffer += (size_t)length;
                    remaining -= (size_t)length;
                }
                break;
        }
    }

    return true;
}

}  // namespace espMeshFlood
