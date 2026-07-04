#include <gtest/gtest.h>
#include "espMeshFlood/serialization/message_serializer.h"
#include "espMeshFlood/types.h"

using namespace espMeshFlood;

class SerializationTest : public ::testing::Test {
protected:
    uint8_t buffer[256];
};

// CT-10: Serialization Protobuf
TEST_F(SerializationTest, SerializeDeserializeBasicMessage) {
    MeshMessage original;
    original.message_id = 0x1234567890ABCDEF;
    original.sender_id = 0x11111111;
    original.timestamp = 0x0000000112345678;
    original.type = MessageType::USER_MESSAGE;
    original.ttl = 5;
    original.payload = {0xAA, 0xBB, 0xCC, 0xDD};

    // Serialize
    size_t serialized_size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(serialized_size, 0);

    // Deserialize
    MeshMessage deserialized;
    bool success = MessageSerializer::deserialize(buffer, serialized_size, deserialized);
    EXPECT_TRUE(success);

    // Verify all fields match
    EXPECT_EQ(deserialized.message_id, original.message_id);
    EXPECT_EQ(deserialized.sender_id, original.sender_id);
    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.ttl, original.ttl);
    EXPECT_EQ(deserialized.payload, original.payload);
}

TEST_F(SerializationTest, EmptyPayload) {
    MeshMessage original;
    original.message_id = 100;
    original.sender_id = 200;
    original.timestamp = 300;
    original.type = MessageType::HEARTBEAT;
    original.ttl = 3;
    // payload is empty

    size_t serialized_size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(serialized_size, 0);

    MeshMessage deserialized;
    bool success = MessageSerializer::deserialize(buffer, serialized_size, deserialized);
    EXPECT_TRUE(success);

    EXPECT_EQ(deserialized.message_id, original.message_id);
    EXPECT_EQ(deserialized.sender_id, original.sender_id);
    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.ttl, original.ttl);
    EXPECT_TRUE(deserialized.payload.empty());
}

TEST_F(SerializationTest, LargePayload) {
    MeshMessage original;
    original.message_id = 999;
    original.sender_id = 888;
    original.timestamp = 777;
    original.type = MessageType::USER_MESSAGE;
    original.ttl = 4;
    
    // Create payload with pattern
    original.payload.resize(150);
    for (size_t i = 0; i < original.payload.size(); i++) {
        original.payload[i] = (uint8_t)(i % 256);
    }

    size_t serialized_size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(serialized_size, 0);

    MeshMessage deserialized;
    bool success = MessageSerializer::deserialize(buffer, serialized_size, deserialized);
    EXPECT_TRUE(success);

    EXPECT_EQ(deserialized.payload.size(), original.payload.size());
    EXPECT_EQ(deserialized.payload, original.payload);
}

TEST_F(SerializationTest, ZeroValues) {
    MeshMessage original;
    original.message_id = 0;
    original.sender_id = 0;
    original.timestamp = 0;
    original.type = MessageType::USER_MESSAGE;
    original.ttl = 0;

    size_t serialized_size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(serialized_size, 0);

    MeshMessage deserialized;
    bool success = MessageSerializer::deserialize(buffer, serialized_size, deserialized);
    EXPECT_TRUE(success);

    EXPECT_EQ(deserialized.message_id, 0);
    EXPECT_EQ(deserialized.sender_id, 0);
    EXPECT_EQ(deserialized.timestamp, 0);
    EXPECT_EQ(deserialized.ttl, 0);
}

TEST_F(SerializationTest, MaxValues) {
    MeshMessage original;
    original.message_id = 0xFFFFFFFFFFFFFFFF;
    original.sender_id = 0xFFFFFFFF;
    original.timestamp = 0xFFFFFFFFFFFFFFFF;
    original.type = MessageType::HEARTBEAT;
    original.ttl = 255;

    size_t serialized_size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(serialized_size, 0);

    MeshMessage deserialized;
    bool success = MessageSerializer::deserialize(buffer, serialized_size, deserialized);
    EXPECT_TRUE(success);

    EXPECT_EQ(deserialized.message_id, original.message_id);
    EXPECT_EQ(deserialized.sender_id, original.sender_id);
    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_EQ(deserialized.ttl, original.ttl);
}

TEST_F(SerializationTest, BufferTooSmall) {
    MeshMessage original;
    original.message_id = 12345;
    original.payload.resize(100);

    uint8_t small_buffer[10];
    size_t serialized_size = MessageSerializer::serialize(original, small_buffer, sizeof(small_buffer));
    
    // Should return 0 or handle gracefully
    EXPECT_LE(serialized_size, (size_t)10);
}

TEST_F(SerializationTest, MalformedInput) {
    uint8_t malformed[] = {0xFF, 0xFF, 0xFF};
    
    MeshMessage msg;
    // Should not crash on malformed input
    bool success = MessageSerializer::deserialize(malformed, sizeof(malformed), msg);
    // May succeed or fail, but should not crash
}

TEST_F(SerializationTest, MultipleRoundTrips) {
    MeshMessage original;
    original.message_id = 0x1234567890ABCDEF;
    original.sender_id = 0xDEADBEEF;
    original.timestamp = 0x0123456789ABCDEF;
    original.type = MessageType::USER_MESSAGE;
    original.ttl = 7;
    original.payload = {0x01, 0x02, 0x03, 0x04};

    MeshMessage current = original;
    
    // Multiple serialize-deserialize cycles
    for (int i = 0; i < 3; i++) {
        size_t size = MessageSerializer::serialize(current, buffer, sizeof(buffer));
        EXPECT_GT(size, 0);
        
        MeshMessage next;
        bool success = MessageSerializer::deserialize(buffer, size, next);
        EXPECT_TRUE(success);
        
        current = next;
    }

    // Should match original after all cycles
    EXPECT_EQ(current.message_id, original.message_id);
    EXPECT_EQ(current.sender_id, original.sender_id);
    EXPECT_EQ(current.timestamp, original.timestamp);
    EXPECT_EQ(current.type, original.type);
    EXPECT_EQ(current.ttl, original.ttl);
    EXPECT_EQ(current.payload, original.payload);
}
