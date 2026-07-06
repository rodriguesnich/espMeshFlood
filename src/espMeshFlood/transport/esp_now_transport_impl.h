#ifndef ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_IMPL_H
#define ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_IMPL_H

#include "esp_now_transport.h"
#include <cstdint>
#include <functional>

namespace espMeshFlood {

/**
 * @brief Real ESP-NOW transport implementation for ESP32
 * 
 * Initializes and manages ESP-NOW communication hardware.
 * Provides initialization, send, and receive capabilities.
 */
class EspNowTransportImpl : public EspNowTransport {
public:
    EspNowTransportImpl();
    ~EspNowTransportImpl();

    /**
     * @brief Initialize ESP-NOW hardware
     * @return true if initialization succeeded
     */
    bool init() override;

    /**
     * @brief Deinitialize ESP-NOW hardware
     */
    void deinit() override;

    /**
     * @brief Send a broadcast message over ESP-NOW
     * @param data Pointer to message data
     * @param length Length of message data
     * @return true if send succeeded, false otherwise
     */
    bool send_broadcast(const uint8_t* data, size_t length) override;

    /**
     * @brief Register a callback for receiving messages
     * @param callback Function to call when message is received
     */
    void register_receive_callback(ReceiveCallback callback) override;

    /**
     * @brief Get the local node ID (extracted from MAC address)
     * @return 32-bit node ID (last 4 bytes of MAC)
     */
    uint32_t get_node_id() const override;

    /**
     * @brief Check if transport is initialized
     * @return true if transport is ready
     */
    bool is_initialized() const override;

    /**
     * @brief Called internally when data is received (callback delegation)
     * @param data Received data
     * @param length Data length
     * @param rssi Signal strength
     */
    void on_data_received(const uint8_t* data, size_t length, int32_t rssi);

private:
    bool initialized_;
    uint32_t node_id_;
    ReceiveCallback receive_callback_;

    /**
     * @brief Extract node ID from MAC address
     * @return 32-bit node ID (last 4 bytes)
     */
    uint32_t extract_node_id_from_mac() const;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_TRANSPORT_ESP_NOW_TRANSPORT_IMPL_H
