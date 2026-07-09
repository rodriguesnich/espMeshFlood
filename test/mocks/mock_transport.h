#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <mutex>
#include <deque>
#include "espMeshFlood/transport/esp_now_transport.h"

namespace espMeshFlood {
namespace test {

class MockEspNowTransport : public EspNowTransport {
public:
    using ReceiveCallback = ::espMeshFlood::ReceiveCallback;

    explicit MockEspNowTransport(uint32_t node_id = 0xAAAAAAAA) : node_id_(node_id) {}

    bool init() override { initialized_ = true; return true; }
    void deinit() override { initialized_ = false; }

    bool send_broadcast(const uint8_t* data, size_t length) override {
        if (!initialized_) return false;
        std::lock_guard<std::mutex> lk(m_);
        sent_messages_.emplace_back(data, data + length);
        return true;
    }

    void register_receive_callback(ReceiveCallback callback) override {
        receive_callback_ = callback;
    }

    uint32_t get_node_id() const override { return node_id_; }
    bool is_initialized() const override { return initialized_; }

    // Test helpers
    size_t get_sent_message_count() const { std::lock_guard<std::mutex> lk(m_); return sent_messages_.size(); }
    std::vector<uint8_t> get_sent_message(size_t idx) const { std::lock_guard<std::mutex> lk(m_); return sent_messages_.at(idx); }

    // Simulate receiving raw data from the radio. Calls the registered callback if present.
    void simulate_receive(const uint8_t* data, size_t length, int32_t rssi) {
        if (receive_callback_) {
            // copy into vector and pass to callback through transport interface
            std::vector<uint8_t> v(data, data + length);
            receive_callback_(v.data(), v.size(), rssi);
        }
    }

private:
    uint32_t node_id_;
    bool initialized_ = false;
    ReceiveCallback receive_callback_ = nullptr;
    mutable std::mutex m_;
    std::deque<std::vector<uint8_t>> sent_messages_;
};

} // namespace test
} // namespace espMeshFlood
