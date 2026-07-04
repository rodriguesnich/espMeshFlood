#include <gtest/gtest.h>
#include "espMeshFlood/message/message_metadata.h"

using namespace espMeshFlood;

class MessageMetadataTest : public ::testing::Test {
protected:
    void SetUp() override {
        metadata = std::make_unique<MessageMetadata>(5);
    }

    std::unique_ptr<MessageMetadata> metadata;
};

TEST_F(MessageMetadataTest, InitialTTL) {
    EXPECT_EQ(metadata->get_ttl(), 5);
}

TEST_F(MessageMetadataTest, InitialRelayCount) {
    EXPECT_EQ(metadata->get_relay_count(), 0);
}

TEST_F(MessageMetadataTest, CanRetransmitWithTTL) {
    EXPECT_TRUE(metadata->can_retransmit());
}

TEST_F(MessageMetadataTest, NotExpiredWithTTL) {
    EXPECT_FALSE(metadata->is_expired());
}

TEST_F(MessageMetadataTest, DecrementTTL) {
    uint32_t new_ttl = metadata->decrement_ttl();
    EXPECT_EQ(new_ttl, 4);
    EXPECT_EQ(metadata->get_ttl(), 4);
}

TEST_F(MessageMetadataTest, IncrementRelayCount) {
    uint32_t new_count = metadata->increment_relay_count();
    EXPECT_EQ(new_count, 1);
    EXPECT_EQ(metadata->get_relay_count(), 1);
}

TEST_F(MessageMetadataTest, TTLDecrementToZero) {
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(metadata->can_retransmit());
        metadata->decrement_ttl();
    }
    
    EXPECT_EQ(metadata->get_ttl(), 0);
    EXPECT_FALSE(metadata->can_retransmit());
    EXPECT_TRUE(metadata->is_expired());
}

TEST_F(MessageMetadataTest, TTLZeroBlocksRetransmission) {
    metadata->set_ttl(0);
    EXPECT_FALSE(metadata->can_retransmit());
    EXPECT_TRUE(metadata->is_expired());
}

TEST_F(MessageMetadataTest, RelayCountIncrement) {
    for (int i = 1; i <= 5; i++) {
        metadata->increment_relay_count();
        EXPECT_EQ(metadata->get_relay_count(), (uint32_t)i);
    }
}

TEST_F(MessageMetadataTest, SetTTL) {
    metadata->set_ttl(10);
    EXPECT_EQ(metadata->get_ttl(), 10);
}

TEST_F(MessageMetadataTest, SetRelayCount) {
    metadata->set_relay_count(7);
    EXPECT_EQ(metadata->get_relay_count(), 7);
}

TEST_F(MessageMetadataTest, MultipleDecrements) {
    metadata->decrement_ttl();
    metadata->decrement_ttl();
    EXPECT_EQ(metadata->get_ttl(), 3);
    
    metadata->decrement_ttl();
    metadata->decrement_ttl();
    EXPECT_EQ(metadata->get_ttl(), 1);
    
    metadata->decrement_ttl();
    EXPECT_EQ(metadata->get_ttl(), 0);
    EXPECT_TRUE(metadata->is_expired());
}

TEST_F(MessageMetadataTest, TTLZeroMinusDecrement) {
    metadata->set_ttl(0);
    
    // Decrementing TTL=0 should stay at 0
    uint32_t result = metadata->decrement_ttl();
    EXPECT_EQ(result, 0);
    EXPECT_EQ(metadata->get_ttl(), 0);
}
