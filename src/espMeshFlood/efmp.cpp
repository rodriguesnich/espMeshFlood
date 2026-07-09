#include "espMeshFlood/efmp.h"
#include "espMeshFlood/protocol/mesh_protocol.h"
#include "espMeshFlood/transport/esp_now_transport.h"
#include "espMeshFlood/transport/esp_now_transport_impl.h"
#include "espMeshFlood/types.h"
#include <memory>

namespace espMeshFlood {

// Provide a mock transport only for unit testing and native builds.
#if defined(UNIT_TESTING) || !defined(ESP32)
class MockEspNowTransport : public EspNowTransport {
public:
    bool init() override {
        initialized_ = true;
        return true;
    }

    void deinit() override {
        initialized_ = false;
    }

    bool send_broadcast(const uint8_t* data, size_t length) override {
        if (!initialized_) return false;
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

private:
    bool initialized_ = false;
    ReceiveCallback receive_callback_;
    uint32_t node_id_ = 0x12345678;
};
#endif

EspMeshFlood& EspMeshFlood::instance() {
    static EspMeshFlood instance;
    return instance;
}

bool EspMeshFlood::init(MessageReceivedCallback callback, std::shared_ptr<EspNowTransport> transport) {
    if (initialized_) {
        return true;  // Already initialized
    }

    // Create transport if not provided
    if (!transport) {
    // Default runtime transport: use real ESP-NOW on ESP32, otherwise use mock for testing/native
#if defined(ESP32) && !defined(UNIT_TESTING)
    transport = std::make_shared<EspNowTransportImpl>();
#else
    transport = std::make_shared<MockEspNowTransport>();
#endif
    }
    transport_ = transport;

    // Create protocol
    protocol_ = std::make_shared<MeshProtocol>(transport_, callback);

    // Initialize protocol
    if (!protocol_->init()) {
        protocol_.reset();
        transport_.reset();
        return false;
    }

    initialized_ = true;
    return true;
}

void EspMeshFlood::deinit() {
    if (protocol_) {
        protocol_->deinit();
        protocol_.reset();
    }
    if (transport_) {
        transport_.reset();
    }
    initialized_ = false;
}

bool EspMeshFlood::send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl) {
    if (!initialized_ || !protocol_) {
        return false;
    }
    return protocol_->send_message(payload, payload_size, ttl);
}

uint32_t EspMeshFlood::get_node_id() const {
    if (!protocol_) {
        return 0;
    }
    return protocol_->get_node_id();
}

bool EspMeshFlood::is_initialized() const {
    return initialized_;
}

void EspMeshFlood::do_maintenance() {
    if (protocol_) {
        protocol_->do_maintenance();
    }
}

void EspMeshFlood::update_time(uint64_t current_time_ms) {
    if (protocol_) {
        protocol_->update_time(current_time_ms);
    }
}

}  // namespace espMeshFlood
