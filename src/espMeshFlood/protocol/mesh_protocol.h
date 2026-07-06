#ifndef ESPMESHFLOOD_PROTOCOL_MESH_PROTOCOL_H
#define ESPMESHFLOOD_PROTOCOL_MESH_PROTOCOL_H

#include "espMeshFlood/config.h"
#include "espMeshFlood/cache/message_cache.h"
#include "espMeshFlood/message/message_metadata.h"
#include "espMeshFlood/backoff/random_backoff.h"
#include "espMeshFlood/transport/esp_now_transport.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace espMeshFlood {

// Forward declaration
struct MeshMessage;

// Typedef for application callback
using ApplicationCallback = std::function<void(const MeshMessage& message, int32_t rssi)>;

/**
 * @brief Core mesh protocol implementation
 * 
 * Implements the EFMP flood gossip protocol:
 * 1. Receive message from transport
 * 2. Check deduplication cache
 * 3. Deliver to application if new
 * 4. Register in cache
 * 5. Schedule retransmission if TTL > 0
 * 6. After random delay, decrement TTL and retransmit
 */
class MeshProtocol {
public:
    /**
     * @brief Initialize protocol with transport and configuration
     * @param transport Transport layer implementation
     * @param app_callback Application callback for received messages
     */
    MeshProtocol(std::shared_ptr<EspNowTransport> transport, ApplicationCallback app_callback);

    /**
     * @brief Initialize the protocol
     * @return true if initialization succeeded
     */
    bool init();

    /**
     * @brief Deinitialize the protocol
     */
    void deinit();

    /**
     * @brief Send a message from this node
     * @param payload Payload bytes
     * @param payload_size Size of payload
     * @param ttl Time to live (default: Config::TTL_INITIAL)
     * @return true if send initiated successfully
     */
    bool send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl = Config::TTL_INITIAL);

    /**
     * @brief Process a received message from the transport layer
     * Called by transport callback.
     * @param data Serialized message data
     * @param length Length of data
     * @param rssi Signal strength
     */
    void on_message_received(const uint8_t* data, size_t length, int32_t rssi);

    /**
     * @brief Get current local node ID
     * @return Node ID
     */
    uint32_t get_node_id() const;

    /**
     * @brief Update system time for cache cleanup
     * @param current_time_ms Current time in milliseconds
     */
    void update_time(uint64_t current_time_ms);

    /**
     * @brief Perform periodic maintenance (cleanup cache, etc)
     */
    void do_maintenance();

private:
    std::shared_ptr<EspNowTransport> transport_;
    ApplicationCallback app_callback_;
    std::unique_ptr<MessageCache> cache_;
    std::unique_ptr<RandomBackoff> backoff_;
    uint64_t current_time_ms_;
    uint32_t message_counter_;

    /**
     * @brief Generate unique message ID
     * @return New message ID
     */
    uint32_t generate_message_id();

    /**
     * @brief Schedule retransmission of a message
     * @param message The message to retransmit
     */
    void schedule_retransmit(const MeshMessage& message);

    /**
     * @brief Internal method to actually retransmit a message
     * @param message The message to retransmit
     */
    void do_retransmit(const MeshMessage& message);
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_PROTOCOL_MESH_PROTOCOL_H
