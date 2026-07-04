#ifndef ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_H
#define ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_H

#include <cstdint>
#include <functional>
#include <vector>

namespace espMeshFlood {

// Typedef for receive callback
using ReceiveCallback = std::function<void(const uint8_t* data, size_t length, int32_t rssi)>;

/**
 * @brief ESP-NOW transport abstraction
 * 
 * Provides initialization, send, and receive capabilities over ESP-NOW.
 * Designed to be mockable for testing.
 */
class EspNowTransport {
public:
    virtual ~EspNowTransport() = default;

    /**
     * @brief Initialize the transport layer
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitialize the transport layer
     */
    virtual void deinit() = 0;

    /**
     * @brief Send a broadcast message over ESP-NOW
     * @param data Pointer to message data
     * @param length Length of message data
     * @return true if send succeeded, false otherwise
     */
    virtual bool send_broadcast(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Register a callback for receiving messages
     * @param callback Function to call when message is received
     */
    virtual void register_receive_callback(ReceiveCallback callback) = 0;

    /**
     * @brief Get the local node ID (MAC address or similar)
     * @return 32-bit node ID
     */
    virtual uint32_t get_node_id() const = 0;

    /**
     * @brief Check if transport is initialized
     * @return true if transport is ready, false otherwise
     */
    virtual bool is_initialized() const = 0;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_H
