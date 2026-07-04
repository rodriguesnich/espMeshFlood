#include <gtest/gtest.h>
#include "espMeshFlood/cache/message_cache.h"

using namespace espMeshFlood;

class MessageCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<MessageCache>(10, 10000);  // 10 entries, 10 second expiration
    }

    std::unique_ptr<MessageCache> cache;
};

// CT-05: Deduplication Basics
TEST_F(MessageCacheTest, SingleMessageNotInCache) {
    uint64_t msg_id = 12345;
    EXPECT_FALSE(cache->exists(msg_id));
}

TEST_F(MessageCacheTest, AddMessageThenExists) {
    uint64_t msg_id = 12345;
    cache->add(msg_id, 0);
    EXPECT_TRUE(cache->exists(msg_id));
}

TEST_F(MessageCacheTest, MultipleMessagesTracked) {
    cache->add(100, 0);
    cache->add(200, 0);
    cache->add(300, 0);
    
    EXPECT_TRUE(cache->exists(100));
    EXPECT_TRUE(cache->exists(200));
    EXPECT_TRUE(cache->exists(300));
    EXPECT_FALSE(cache->exists(400));
}

// CT-05: Duplicate rejection
TEST_F(MessageCacheTest, DuplicateMessageRejected) {
    uint64_t msg_id = 12345;
    
    // First add
    cache->add(msg_id, 0);
    EXPECT_TRUE(cache->exists(msg_id));
    
    // Adding same ID again should still report exists
    cache->add(msg_id, 0);
    EXPECT_TRUE(cache->exists(msg_id));
    
    // Cache size should not grow
    EXPECT_EQ(cache->size(), 1);
}

// Cache expiration
TEST_F(MessageCacheTest, ExpiredEntriesRemoved) {
    uint64_t msg_id = 12345;
    uint64_t current_time = 0;
    
    cache->add(msg_id, current_time);
    EXPECT_TRUE(cache->exists(msg_id));
    
    // Advance time past expiration (10 seconds in this case)
    current_time = 11000;
    cache->cleanup_expired(current_time);
    
    EXPECT_FALSE(cache->exists(msg_id));
}

// Cache bounded size
TEST_F(MessageCacheTest, CacheBoundedSize) {
    MessageCache small_cache(3, 10000);
    
    small_cache.add(100, 0);
    small_cache.add(200, 0);
    small_cache.add(300, 0);
    EXPECT_EQ(small_cache.size(), 3);
    
    // Add one more - should evict oldest
    small_cache.add(400, 0);
    EXPECT_EQ(small_cache.size(), 3);
    
    // First entry should be evicted
    EXPECT_FALSE(small_cache.exists(100));
    EXPECT_TRUE(small_cache.exists(200));
    EXPECT_TRUE(small_cache.exists(300));
    EXPECT_TRUE(small_cache.exists(400));
}

TEST_F(MessageCacheTest, ClearRemovesAllEntries) {
    cache->add(100, 0);
    cache->add(200, 0);
    cache->add(300, 0);
    EXPECT_EQ(cache->size(), 3);
    
    cache->clear();
    EXPECT_EQ(cache->size(), 0);
    EXPECT_FALSE(cache->exists(100));
    EXPECT_FALSE(cache->exists(200));
    EXPECT_FALSE(cache->exists(300));
}

TEST_F(MessageCacheTest, MixedExpirationAndCapacity) {
    MessageCache mixed_cache(5, 1000);
    
    // Add 5 entries at time 0
    mixed_cache.add(100, 0);
    mixed_cache.add(200, 0);
    mixed_cache.add(300, 0);
    mixed_cache.add(400, 0);
    mixed_cache.add(500, 0);
    EXPECT_EQ(mixed_cache.size(), 5);
    
    // Clean up at time 500 (all still valid)
    mixed_cache.cleanup_expired(500);
    EXPECT_EQ(mixed_cache.size(), 5);
    
    // Clean up at time 1500 (all expired)
    mixed_cache.cleanup_expired(1500);
    EXPECT_EQ(mixed_cache.size(), 0);
}
