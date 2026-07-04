#ifndef ESPMESHFLOOD_SERIALIZATION_MESSAGE_SERIALIZER_H
#define ESPMESHFLOOD_SERIALIZATION_MESSAGE_SERIALIZER_H

#include <cstdint>
#include <vector>
#include <cstring>

namespace espMeshFlood {

// Forward declaration
struct MeshMessage;

/**
 * @brief Message serialization and deserialization
 * 
 * Handles encoding/decoding of MeshMessage using Protobuf (nanopb).
 */
class MessageSerializer {
public:
    /**
     * @brief Serialize a MeshMessage to bytes
     * @param message The message to serialize
     * @param buffer Output buffer
     * @param buffer_size Size of output buffer
     * @return Number of bytes written, or 0 on error
     */
    static size_t serialize(const MeshMessage& message, uint8_t* buffer, size_t buffer_size);

    /**
     * @brief Deserialize bytes to a MeshMessage
     * @param buffer Input buffer containing serialized message
     * @param buffer_size Size of input buffer
     * @param message Output message structure
     * @return true if deserialization succeeded, false otherwise
     */
    static bool deserialize(const uint8_t* buffer, size_t buffer_size, MeshMessage& message);

    /**
     * @brief Get maximum serialized size of a MeshMessage
     * @return Maximum bytes needed for serialization
     */
    static constexpr size_t max_size() {
        return 256;  // Conservative estimate for MeshMessage
    }
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_SERIALIZATION_MESSAGE_SERIALIZER_H
