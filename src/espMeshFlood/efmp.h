#ifndef ESPMESHFLOOD_EFMP_H
#define ESPMESHFLOOD_EFMP_H

#include "espMeshFlood/types.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace espMeshFlood {

// Forward declarations
struct MeshMessage;
class EspNowTransport;
class MeshProtocol;

// Typedef for application callback
using MessageReceivedCallback = std::function<void(const MeshMessage& message, int32_t rssi)>;

/**
 * @brief Public API for ESP Mesh Flood Protocol (EFMP)
 * 
 * Single entry point for using the mesh protocol in applications.
 * Handles initialization, sending, and receiving messages.
 */
class EspMeshFlood {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to EspMeshFlood instance
     */
    static EspMeshFlood& instance();

    /**
     * @brief Initialize the mesh protocol
     * @param callback Application callback for received messages
     * @param transport Transport layer (optional, default creates ESP-NOW transport)
     * @return true if initialization succeeded
     */
    bool init(MessageReceivedCallback callback, std::shared_ptr<EspNowTransport> transport = nullptr);

    /**
     * @brief Deinitialize the mesh protocol
     */
    void deinit();

    /**
     * @brief Send a message through the mesh
     * @param payload Message payload
     * @param payload_size Size of payload in bytes
     * @param ttl Time to live (default: 5)
     * @param type Message type (default: USER_MESSAGE)
     * @return true if send initiated successfully
     */
    bool send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl = 5,
                      MessageType type = MessageType::USER_MESSAGE);

    /**
     * @brief Get the local node ID
     * @return 32-bit node ID
     */
    uint32_t get_node_id() const;

    /**
     * @brief Check if protocol is initialized
     * @return true if ready to send/receive
     */
    bool is_initialized() const;

    /**
     * @brief Perform periodic maintenance
     * Should be called regularly (e.g., from main loop)
     */
    void do_maintenance();

    /**
     * @brief Update system time
     * @param current_time_ms Current time in milliseconds
     */
    void update_time(uint64_t current_time_ms);

private:
    EspMeshFlood() = default;
    ~EspMeshFlood() = default;

    // Delete copy constructor and assignment operator
    EspMeshFlood(const EspMeshFlood&) = delete;
    EspMeshFlood& operator=(const EspMeshFlood&) = delete;

    std::shared_ptr<MeshProtocol> protocol_;
    std::shared_ptr<EspNowTransport> transport_;
    bool initialized_;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_EFMP_H
