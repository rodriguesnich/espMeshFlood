#include "espMeshFlood/protocol/mesh_protocol.h"
#include "espMeshFlood/types.h"
#include "espMeshFlood/serialization/message_serializer.h"
#include "espMeshFlood/config.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <cstring>

namespace espMeshFlood {

MeshProtocol::MeshProtocol(std::shared_ptr<EspNowTransport> transport, ApplicationCallback app_callback)
    : transport_(transport), app_callback_(app_callback), 
    cache_(std::unique_ptr<MessageCache>(new MessageCache(Config::CACHE_SIZE, Config::CACHE_ENTRY_EXPIRATION_MS))),
    backoff_(std::unique_ptr<RandomBackoff>(new RandomBackoff(Config::MIN_RELAY_DELAY_MS, Config::MAX_RELAY_DELAY_MS))),
      current_time_ms_(0), message_counter_(0) {
}

bool MeshProtocol::init() {
    if (!transport_) {
        return false;
    }

    if (!transport_->init()) {
        return false;
    }

    // Register our callback with the transport
    transport_->register_receive_callback([this](const uint8_t* data, size_t length, int32_t rssi) {
        this->on_message_received(data, length, rssi);
    });

    return true;
}

void MeshProtocol::deinit() {
    if (transport_) {
        transport_->deinit();
    }
    if (cache_) {
        cache_->clear();
    }
    pending_retransmissions_.clear();
}

bool MeshProtocol::send_message(const uint8_t* payload, size_t payload_size, uint32_t ttl,
                                  MessageType type) {
    if (!transport_ || !transport_->is_initialized()) {
        return false;
    }

    if (payload_size > Config::MAX_MESSAGE_PAYLOAD_SIZE) {
        return false;
    }

    // Create message
    MeshMessage message;
    message.origin_node_id = transport_->get_node_id();
    message.message_id = generate_message_id();
    message.sender_id = transport_->get_node_id();
    message.timestamp = current_time_ms_;
    message.type = type;
    message.ttl = ttl;
    message.payload.resize(payload_size);
    std::memcpy(message.payload.data(), payload, payload_size);

    // Register in cache immediately
    cache_->add(message.sender_id, message.message_id, current_time_ms_);

    // Serialize and send
    uint8_t buffer[MessageSerializer::max_size()];
    size_t serialized_size = MessageSerializer::serialize(message, buffer, sizeof(buffer));
    if (serialized_size == 0) {
        return false;
    }

    bool success = transport_->send_broadcast(buffer, serialized_size);
    
    if (success && app_callback_) {
        // Deliver to own application
        app_callback_(message, 0);
    }

    return success;
}

void MeshProtocol::on_message_received(const uint8_t* data, size_t length, int32_t rssi) {
    // Deserialize message
    MeshMessage message;
    if (!MessageSerializer::deserialize(data, length, message)) {
        return;  // Invalid message
    }

    // Check if already processed (deduplication)
    if (cache_->exists(message.sender_id, message.message_id, current_time_ms_)) {
        return;  // Already seen, discard
    }

    // Register in cache immediately
    cache_->add(message.sender_id, message.message_id, current_time_ms_);

    // Deliver to application if callback is registered
    if (app_callback_) {
        app_callback_(message, rssi);
    }

    // Check if we should retransmit
    if (message.ttl > 0) {
        // Schedule retransmission after random delay
        schedule_retransmit(message);
    }
}

uint32_t MeshProtocol::get_node_id() const {
    if (transport_) {
        return transport_->get_node_id();
    }
    return 0;
}

void MeshProtocol::update_time(uint64_t current_time_ms) {
    current_time_ms_ = current_time_ms;
}

void MeshProtocol::do_maintenance() {
    cache_->cleanup_expired(current_time_ms_);

    // Dispatch any retransmissions that are due at the current time.
    for (auto it = pending_retransmissions_.begin(); it != pending_retransmissions_.end();) {
        if (it->due_time_ms <= current_time_ms_) {
            do_retransmit(it->message);
            it = pending_retransmissions_.erase(it);
            continue;
        }
        ++it;
    }
}

uint32_t MeshProtocol::generate_message_id() {
    // Message ID is now a 32-bit local counter.
    return message_counter_++;
}

void MeshProtocol::schedule_retransmit(const MeshMessage& message) {
    const uint32_t delay_ms = backoff_->get_delay();
    PendingRetransmission pending{message, current_time_ms_ + static_cast<uint64_t>(delay_ms)};
    pending_retransmissions_.push_back(std::move(pending));
}

void MeshProtocol::do_retransmit(const MeshMessage& message) {
    if (!transport_ || message.ttl == 0) {
        return;
    }

    // Create retransmission copy
    MeshMessage retransmit_msg = message;
    retransmit_msg.ttl--;

    // Serialize and send. TTL may be zero here by design (last propagation hop).
    uint8_t buffer[MessageSerializer::max_size()];
    size_t serialized_size = MessageSerializer::serialize(retransmit_msg, buffer, sizeof(buffer));
    if (serialized_size > 0) {
        transport_->send_broadcast(buffer, serialized_size);
    }
}

}  // namespace espMeshFlood
