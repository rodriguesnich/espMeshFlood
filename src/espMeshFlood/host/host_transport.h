#ifndef ESPMESHFLOOD_HOST_HOST_TRANSPORT_H
#define ESPMESHFLOOD_HOST_HOST_TRANSPORT_H

#include <cstdint>
#include <functional>

namespace espMeshFlood {

/**
 * @brief Callback type for data received from the host (post-framing stripped).
 *
 * Delivers the raw protobuf payload bytes of a complete, validated frame.
 * The pointer is valid only for the duration of the callback.
 */
using HostReceiveCallback = std::function<void(const uint8_t* data, size_t length)>;

/**
 * @brief Abstract interface for host-facing communication transports.
 *
 * Mirrors the EspNowTransport pattern for the host (external client) side.
 * Concrete implementations: SerialHostTransport, BleHostTransport.
 *
 * Wire protocol (shared by all implementations):
 *   [0xAA magic: 1 byte][length: 2 bytes LE][protobuf payload: N bytes]
 */
class IHostTransport {
public:
    virtual ~IHostTransport() = default;

    /**
     * @brief Initialize the transport layer.
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * @brief Send a protobuf payload to the connected host.
     *
     * Implementations wrap data in the standard frame before transmitting.
     * @param data   Protobuf-serialized MeshMessage bytes
     * @param length Number of bytes in data
     */
    virtual void send(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Register a callback invoked when a complete inbound frame is received.
     *
     * The callback receives the raw protobuf payload (framing stripped).
     */
    virtual void register_receive_callback(HostReceiveCallback cb) = 0;

    /**
     * @brief Check whether a host client is currently connected.
     *
     * Always true for serial; reflects BLE connection state for BleHostTransport.
     */
    virtual bool is_connected() const = 0;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_HOST_HOST_TRANSPORT_H
