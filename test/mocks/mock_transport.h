#ifndef ESPMESHFLOOD_TEST_MOCKS_MOCK_TRANSPORT_H
#define ESPMESHFLOOD_TEST_MOCKS_MOCK_TRANSPORT_H

#include "../src/espMeshFlood/transport/esp_now_transport.h"
#include <vector>
#include <deque>

namespace espMeshFlood {
namespace test {

/**
 * @brief Mock ESP-NOW transport for testing
 * 
 * Records sent messages and allows simulating received messages.
 */
class MockEspNowTransport : public EspNowTransport {
public:
    MockEspNowTransport(uint32_t node_id = 0x11111111) : node_id_(node_id) {}

    bool init() override {
        initialized_ = true;
        return true;
    }

    void deinit() override {
        initialized_ = false;
    }

    bool send_broadcast(const uint8_t* data, size_t length) override {
        if (!initialized_) return false;
        
        std::vector<uint8_t> message(data, data + length);
        sent_messages_.push_back(message);
        return true;
    }

    void register_receive_callback(ReceiveCallback callback) override {
        receive_callback_ = callback;
    }

    uint32_t get_node_id() const override {
        return node_id_;
    }

    bool is_initialized() const override {
        return initialized_;
    }

    // Test helpers
    void simulate_receive(const uint8_t* data, size_t length, int32_t rssi = -50) {
        if (receive_callback_) {
            receive_callback_(data, length, rssi);
        }
    }

    size_t get_sent_message_count() const {
        return sent_messages_.size();
    }

    const std::vector<uint8_t>& get_sent_message(size_t index) const {
        return sent_messages_[index];
    }

    void clear_sent_messages() {
        sent_messages_.clear();
    }

private:
    bool initialized_ = false;
    uint32_t node_id_;
    ReceiveCallback receive_callback_;
    std::vector<std::vector<uint8_t>> sent_messages_;
};

}  // namespace test
}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_TEST_MOCKS_MOCK_TRANSPORT_H
