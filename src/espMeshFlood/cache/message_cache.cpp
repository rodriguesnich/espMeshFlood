#include "espMeshFlood/cache/message_cache.h"
#include <algorithm>

namespace espMeshFlood {

MessageCache::MessageCache(size_t max_size, uint64_t expiration_ms)
    : max_size_(max_size), expiration_ms_(expiration_ms), write_index_(0) {
    entries_.reserve(max_size);
}

bool MessageCache::exists(uint64_t message_id) const {
    for (const auto& entry : entries_) {
        if (entry.message_id == message_id) {
            return true;
        }
    }
    return false;
}

bool MessageCache::exists(uint64_t message_id, uint64_t current_time_ms) {
    cleanup_expired(current_time_ms);
    return exists(message_id);
}

void MessageCache::add(uint64_t message_id, uint64_t current_time_ms) {
    // Check if message already exists to avoid duplicates
    if (exists(message_id, current_time_ms)) {
        return;
    }

    // If cache is full, remove oldest entry
    if (entries_.size() >= max_size_) {
        // Remove first entry (oldest)
        if (!entries_.empty()) {
            entries_.erase(entries_.begin());
        }
    }

    // Add new entry
    CacheEntry entry;
    entry.message_id = message_id;
    entry.timestamp_ms = current_time_ms;
    entries_.push_back(entry);
}

void MessageCache::cleanup_expired(uint64_t current_time_ms) {
    // Remove expired entries
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [this, current_time_ms](const CacheEntry& entry) {
                return (current_time_ms - entry.timestamp_ms) > expiration_ms_;
            }),
        entries_.end()
    );
}

size_t MessageCache::size() const {
    return entries_.size();
}

void MessageCache::clear() {
    entries_.clear();
}

}  // namespace espMeshFlood
