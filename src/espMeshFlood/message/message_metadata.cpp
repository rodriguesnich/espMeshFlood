#include "espMeshFlood/message/message_metadata.h"

namespace espMeshFlood {

MessageMetadata::MessageMetadata(uint32_t ttl)
    : ttl_(ttl), relay_count_(0) {
}

uint32_t MessageMetadata::get_ttl() const {
    return ttl_;
}

uint32_t MessageMetadata::get_relay_count() const {
    return relay_count_;
}

bool MessageMetadata::is_expired() const {
    return ttl_ == 0;
}

bool MessageMetadata::can_retransmit() const {
    return ttl_ > 0;
}

uint32_t MessageMetadata::decrement_ttl() {
    if (ttl_ > 0) {
        ttl_--;
    }
    return ttl_;
}

uint32_t MessageMetadata::increment_relay_count() {
    relay_count_++;
    return relay_count_;
}

void MessageMetadata::set_ttl(uint32_t ttl) {
    ttl_ = ttl;
}

void MessageMetadata::set_relay_count(uint32_t relay_count) {
    relay_count_ = relay_count;
}

}  // namespace espMeshFlood
