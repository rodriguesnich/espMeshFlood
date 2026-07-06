#ifndef ESPMESHFLOOD_TYPES_H
#define ESPMESHFLOOD_TYPES_H

#include <cstdint>
#include <vector>

namespace espMeshFlood {

/**
 * @brief Message type enumeration
 */
enum class MessageType : uint32_t {
    USER_MESSAGE = 0,
    HEARTBEAT = 1,
};

/**
 * @brief Mesh message structure
 * 
 * Represents the core message format propagated through the mesh.
 * Fields correspond to the Protobuf MeshMessage definition.
 */
struct MeshMessage {
    uint32_t message_id;
    uint32_t sender_id;
    uint64_t timestamp;
    MessageType type;
    uint32_t ttl;
    uint32_t relay_count;
    std::vector<uint8_t> payload;

    MeshMessage() 
        : message_id(0), sender_id(0), timestamp(0), type(MessageType::USER_MESSAGE),
          ttl(5), relay_count(0) {}
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_TYPES_H
