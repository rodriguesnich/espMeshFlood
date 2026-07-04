#ifndef ESPMESHFLOOD_CONFIG_H
#define ESPMESHFLOOD_CONFIG_H

#include <cstdint>
#include <cstddef>

namespace espMeshFlood {

/**
 * @brief Configuration constants for EFMP
 */
class Config {
public:
    // TTL Configuration
    static constexpr uint32_t TTL_INITIAL = 5;
    static constexpr uint32_t TTL_MIN = 0;
    static constexpr uint32_t TTL_MAX = 255;

    // Cache Configuration
    static constexpr size_t CACHE_SIZE = 100;
    static constexpr uint64_t CACHE_ENTRY_EXPIRATION_MS = 10 * 60 * 1000;  // 10 minutes

    // Retransmission Backoff Configuration
    static constexpr uint32_t MIN_RELAY_DELAY_MS = 50;
    static constexpr uint32_t MAX_RELAY_DELAY_MS = 300;

    // ESP-NOW Configuration
    static constexpr size_t MAX_PAYLOAD_SIZE = 250;
    static constexpr size_t MAX_MESSAGE_PAYLOAD_SIZE = 200;  // Conservative estimate

    // Message Configuration
    static constexpr size_t MAX_BROADCAST_RETRIES = 5;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_CONFIG_H
