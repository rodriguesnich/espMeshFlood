#ifndef ESPMESHFLOOD_CACHE_MESSAGE_CACHE_H
#define ESPMESHFLOOD_CACHE_MESSAGE_CACHE_H

#include <cstdint>
#include <vector>
#include <cstring>

namespace espMeshFlood {

/**
 * @brief Cache entry for deduplication
 */
struct CacheEntry {
    uint32_t sender_id;
    uint32_t message_id;
    uint64_t timestamp_ms;  // milliseconds when added
};

/**
 * @brief Message deduplication cache
 * 
 * Maintains a circular buffer of recently seen message IDs to prevent
 * duplicate processing. Entries expire after a configured timeout.
 */
class MessageCache {
public:
    /**
     * @brief Initialize the cache with specified size
     * @param max_size Maximum number of entries to store
     * @param expiration_ms Time in milliseconds before entries expire
     */
    explicit MessageCache(size_t max_size, uint64_t expiration_ms);

    /**
    * @brief Check if a sender/message pair exists in cache
    * @param sender_id The sender ID associated with the message
     * @param message_id The message ID to check
    * @return true if the sender/message pair is in cache, false otherwise
     */
    bool exists(uint32_t sender_id, uint32_t message_id) const;

    /**
    * @brief Check if a sender/message pair exists in cache after cleaning expired entries
    * @param sender_id The sender ID associated with the message
     * @param message_id The message ID to check
     * @param current_time_ms Current system time in milliseconds
    * @return true if the sender/message pair is in cache and not expired, false otherwise
     */
    bool exists(uint32_t sender_id, uint32_t message_id, uint64_t current_time_ms);

    /**
    * @brief Add a sender/message pair to the cache
    * @param sender_id The sender ID associated with the message
     * @param message_id The message ID to add
     * @param current_time_ms Current system time in milliseconds
     */
    void add(uint32_t sender_id, uint32_t message_id, uint64_t current_time_ms);

    /**
     * @brief Remove expired entries from the cache
     * @param current_time_ms Current system time in milliseconds
     */
    void cleanup_expired(uint64_t current_time_ms);

    /**
     * @brief Get current number of entries in cache
     * @return Number of active cache entries
     */
    size_t size() const;

    /**
     * @brief Clear all entries from cache
     */
    void clear();

private:
    std::vector<CacheEntry> entries_;
    size_t max_size_;
    uint64_t expiration_ms_;
    size_t write_index_;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_CACHE_MESSAGE_CACHE_H
