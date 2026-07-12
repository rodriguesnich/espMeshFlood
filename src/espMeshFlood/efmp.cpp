#include "espMeshFlood/efmp.h"
#include "espMeshFlood/protocol/mesh_protocol.h"
#include "espMeshFlood/transport/esp_now_transport.h"
#include "espMeshFlood/transport/esp_now_transport_impl.h"
#include "espMeshFlood/types.h"
#include <memory>

namespace espMeshFlood {

// Runtime must use real ESP-NOW transport on ESP32. Mocks for unit tests
// should be provided only by test builds and not included in runtime code.

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
        // Runtime-only behavior: require ESP32 and use real ESP-NOW transport.
#if defined(ESP32)
        transport = std::make_shared<EspNowTransportImpl>();
#else
        // Non-ESP32 native builds must supply a transport (unit tests may
        // inject mocks via the `transport` parameter guarded by UNIT_TESTING).
        return false;
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

bool EspMeshFlood::send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl,
                                 MessageType type) {
    if (!initialized_ || !protocol_) {
        return false;
    }
    return protocol_->send_message(payload, payload_size, ttl, type);
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
