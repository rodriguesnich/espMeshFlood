#ifndef ESPMESHFLOOD_MESSAGE_MESSAGE_METADATA_H
#define ESPMESHFLOOD_MESSAGE_MESSAGE_METADATA_H

#include <cstdint>

namespace espMeshFlood {

/**
 * @brief Metadata for message propagation control
 * 
 * Encapsulates TTL (Time To Live) and relay_count tracking for message
 * propagation through the mesh network.
 */
class MessageMetadata {
public:
    /**
     * @brief Initialize metadata with initial TTL
     * @param ttl Initial TTL value
     */
    explicit MessageMetadata(uint32_t ttl = 5);

    /**
     * @brief Get current TTL value
     * @return Current TTL
     */
    uint32_t get_ttl() const;

    /**
     * @brief Get current relay count
     * @return Number of retransmissions performed
     */
    uint32_t get_relay_count() const;

    /**
     * @brief Check if message has expired (TTL reached zero)
     * @return true if TTL is 0, false otherwise
     */
    bool is_expired() const;

    /**
     * @brief Check if message can be retransmitted (TTL > 0)
     * @return true if TTL > 0, false otherwise
     */
    bool can_retransmit() const;

    /**
     * @brief Decrement TTL by 1
     * @return New TTL value after decrement
     */
    uint32_t decrement_ttl();

    /**
     * @brief Increment relay count by 1
     * @return New relay count after increment
     */
    uint32_t increment_relay_count();

    /**
     * @brief Set TTL to specific value
     * @param ttl New TTL value
     */
    void set_ttl(uint32_t ttl);

    /**
     * @brief Set relay count to specific value
     * @param relay_count New relay count
     */
    void set_relay_count(uint32_t relay_count);

private:
    uint32_t ttl_;
    uint32_t relay_count_;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_MESSAGE_MESSAGE_METADATA_H
