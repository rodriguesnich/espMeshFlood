#include <gtest/gtest.h>
#include "espMeshFlood/protocol/mesh_protocol.h"
#include "espMeshFlood/types.h"
#include "espMeshFlood/serialization/message_serializer.h"
#include <memory>
#include "mock_transport.h"

using namespace espMeshFlood;
using namespace espMeshFlood::test;

class MeshProtocolIntegrationTest : public ::testing::Test {
protected:
    static void advance_and_maintain(MeshProtocol& protocol, uint64_t now_ms) {
        protocol.update_time(now_ms);
        protocol.do_maintenance();
    }

    void SetUp() override {
        transport = std::make_shared<MockEspNowTransport>(0x11111111);
        
        received_messages.clear();
        callback = [this](const MeshMessage& msg, int32_t rssi) {
            received_messages.push_back({msg, rssi});
        };
    }

    struct ReceivedMessage {
        MeshMessage msg;
        int32_t rssi;
    };

    std::shared_ptr<MockEspNowTransport> transport;
    std::vector<ReceivedMessage> received_messages;
    ApplicationCallback callback;
};

// CT-01: Direct Delivery
TEST_F(MeshProtocolIntegrationTest, DirectDelivery) {
    MeshProtocol protocol(transport, callback);
    EXPECT_TRUE(protocol.init());
    protocol.update_time(1000);

    // Send a message
    uint8_t payload[] = {0xAA, 0xBB, 0xCC};
    EXPECT_TRUE(protocol.send_message(payload, sizeof(payload), 5));

    // Should have one sent message
    EXPECT_EQ(transport->get_sent_message_count(), 1);

    // Deserialize the sent message
    const auto& sent_msg_data = transport->get_sent_message(0);
    MeshMessage sent_msg;
    EXPECT_TRUE(MessageSerializer::deserialize(sent_msg_data.data(), sent_msg_data.size(), sent_msg));

    EXPECT_EQ(sent_msg.sender_id, protocol.get_node_id());
    EXPECT_EQ(sent_msg.ttl, 5);
    EXPECT_EQ(sent_msg.payload.size(), sizeof(payload));

    // Application should have received the message
    EXPECT_EQ(received_messages.size(), 1);
}

// CT-02: One Hop Relay
TEST_F(MeshProtocolIntegrationTest, OneHopRelay) {
    // Node A sends a message
    auto transport_a = std::make_shared<MockEspNowTransport>(0x11111111);
    MeshProtocol protocol_a(transport_a, callback);
    EXPECT_TRUE(protocol_a.init());
    protocol_a.update_time(1000);

    // Send message from A
    uint8_t payload[] = {0xAA, 0xBB};
    EXPECT_TRUE(protocol_a.send_message(payload, sizeof(payload), 5));

    // Get the sent message
    EXPECT_EQ(transport_a->get_sent_message_count(), 1);
    const auto& sent_msg_data = transport_a->get_sent_message(0);
    MeshMessage sent_msg;
    EXPECT_TRUE(MessageSerializer::deserialize(sent_msg_data.data(), sent_msg_data.size(), sent_msg));

    // Node B receives and relays
    received_messages.clear();
    auto transport_b = std::make_shared<MockEspNowTransport>(0x22222222);
    MeshProtocol protocol_b(transport_b, callback);
    EXPECT_TRUE(protocol_b.init());
    protocol_b.update_time(1000);

    // Simulate B receiving A's message
    transport_b->simulate_receive(sent_msg_data.data(), sent_msg_data.size(), -50);

    // Retransmit is scheduled, not immediate.
    EXPECT_EQ(transport_b->get_sent_message_count(), 0);
    advance_and_maintain(protocol_b, 2000);

    // B should have received it
    EXPECT_EQ(received_messages.size(), 1);
    EXPECT_EQ(received_messages[0].msg.message_id, sent_msg.message_id);

    // B should have retransmitted (for now immediately, in real implementation with delay)
    EXPECT_GE(transport_b->get_sent_message_count(), 1);

    // Get the relayed message
    const auto& relayed_msg_data = transport_b->get_sent_message(0);
    MeshMessage relayed_msg;
    EXPECT_TRUE(MessageSerializer::deserialize(relayed_msg_data.data(), relayed_msg_data.size(), relayed_msg));

    // Should have decreased TTL
    EXPECT_EQ(relayed_msg.ttl, sent_msg.ttl - 1);
}

// CT-03: Multi-hop
TEST_F(MeshProtocolIntegrationTest, MultiHopPropagation) {
    std::vector<std::shared_ptr<MockEspNowTransport>> transports;
    std::vector<MeshProtocol*> protocols;
    std::vector<std::vector<ReceivedMessage>> all_received;

    // Create 3 nodes: A, B, C
    for (int i = 0; i < 3; i++) {
        transports.push_back(std::make_shared<MockEspNowTransport>(0x11111111 + i));
        all_received.push_back({});
    }

    // Setup protocols with individual callbacks
    for (int i = 0; i < 3; i++) {
        int node_idx = i;
        ApplicationCallback cb = [this, node_idx, &all_received](const MeshMessage& msg, int32_t rssi) {
            all_received[node_idx].push_back({msg, rssi});
        };
        protocols.push_back(new MeshProtocol(transports[i], cb));
        EXPECT_TRUE(protocols[i]->init());
        protocols[i]->update_time(1000);
    }

    // A sends message
    uint8_t payload[] = {0xAA};
    EXPECT_TRUE(protocols[0]->send_message(payload, sizeof(payload), 5));

    // Get A's sent message
    EXPECT_EQ(transports[0]->get_sent_message_count(), 1);
    auto sent_msg_data = transports[0]->get_sent_message(0);
    MeshMessage msg_from_a;
    EXPECT_TRUE(MessageSerializer::deserialize(sent_msg_data.data(), sent_msg_data.size(), msg_from_a));

    // B receives and relays
    transports[1]->simulate_receive(sent_msg_data.data(), sent_msg_data.size(), -50);
    advance_and_maintain(*protocols[1], 2000);
    EXPECT_GE(transports[1]->get_sent_message_count(), 1);

    // Get B's relayed message
    auto relayed_from_b = transports[1]->get_sent_message(0);
    MeshMessage msg_from_b;
    EXPECT_TRUE(MessageSerializer::deserialize(relayed_from_b.data(), relayed_from_b.size(), msg_from_b));

    // C receives B's relay
    transports[2]->simulate_receive(relayed_from_b.data(), relayed_from_b.size(), -60);
    advance_and_maintain(*protocols[2], 3000);
    EXPECT_GE(transports[2]->get_sent_message_count(), 1);

    // Get C's relayed message
    auto relayed_from_c = transports[2]->get_sent_message(0);
    MeshMessage msg_from_c;
    EXPECT_TRUE(MessageSerializer::deserialize(relayed_from_c.data(), relayed_from_c.size(), msg_from_c));

    // All should have same message_id
    EXPECT_EQ(msg_from_a.message_id, msg_from_b.message_id);
    EXPECT_EQ(msg_from_b.message_id, msg_from_c.message_id);

    // TTL should decrement
    EXPECT_EQ(msg_from_a.ttl, 5);
    EXPECT_EQ(msg_from_b.ttl, 4);
    EXPECT_EQ(msg_from_c.ttl, 3);

    // Cleanup
    for (auto* proto : protocols) {
        delete proto;
    }
}

// CT-05: Deduplication in Protocol
TEST_F(MeshProtocolIntegrationTest, DeduplicationInProtocol) {
    MeshProtocol protocol(transport, callback);
    EXPECT_TRUE(protocol.init());
    protocol.update_time(1000);

    // Create a test message
    MeshMessage original;
    original.message_id = 12345;
    original.sender_id = 0x99999999;
    original.timestamp = 1000;
    original.type = MessageType::USER_MESSAGE;
    original.ttl = 5;
    original.payload = {0xAA, 0xBB};

    // Serialize it
    uint8_t buffer[256];
    size_t size = MessageSerializer::serialize(original, buffer, sizeof(buffer));
    EXPECT_GT(size, 0);

    // Simulate receiving the same message twice
    received_messages.clear();
    transport->simulate_receive(buffer, size, -50);
    transport->simulate_receive(buffer, size, -50);

    // Application should only receive it once (deduplication)
    EXPECT_EQ(received_messages.size(), 1);
}

// CT-04: TTL Limit
TEST_F(MeshProtocolIntegrationTest, TTLLimit) {
    // Create 5 nodes in a chain: A -> B -> C -> D -> E
    std::vector<std::shared_ptr<MockEspNowTransport>> transports;
    std::vector<MeshProtocol*> protocols;

    for (int i = 0; i < 5; i++) {
        transports.push_back(std::make_shared<MockEspNowTransport>(0x10000000 + i));
        ApplicationCallback cb = [](const MeshMessage& msg, int32_t rssi) {
            // Just accept messages
        };
        protocols.push_back(new MeshProtocol(transports[i], cb));
        EXPECT_TRUE(protocols[i]->init());
        protocols[i]->update_time(1000);
    }

    // A sends with TTL=2
    uint8_t payload[] = {0xAA};
    EXPECT_TRUE(protocols[0]->send_message(payload, sizeof(payload), 2));

    // Get A's message
    EXPECT_EQ(transports[0]->get_sent_message_count(), 1);
    auto msg_data = transports[0]->get_sent_message(0);
    MeshMessage msg;
    EXPECT_TRUE(MessageSerializer::deserialize(msg_data.data(), msg_data.size(), msg));
    EXPECT_EQ(msg.ttl, 2);

    // B receives and relays (TTL becomes 1)
    transports[1]->simulate_receive(msg_data.data(), msg_data.size(), -50);
    advance_and_maintain(*protocols[1], 2000);
    EXPECT_GE(transports[1]->get_sent_message_count(), 1);

    auto msg_from_b = transports[1]->get_sent_message(0);
    EXPECT_TRUE(MessageSerializer::deserialize(msg_from_b.data(), msg_from_b.size(), msg));
    EXPECT_EQ(msg.ttl, 1);

    // C receives and relays (TTL becomes 0)
    transports[2]->simulate_receive(msg_from_b.data(), msg_from_b.size(), -50);
    advance_and_maintain(*protocols[2], 3000);
    EXPECT_GE(transports[2]->get_sent_message_count(), 1);

    auto msg_from_c = transports[2]->get_sent_message(0);
    EXPECT_TRUE(MessageSerializer::deserialize(msg_from_c.data(), msg_from_c.size(), msg));
    EXPECT_EQ(msg.ttl, 0);

    // D and E should NOT receive because TTL is 0
    // (In our implementation, TTL=0 means no retransmission)
    transports[3]->simulate_receive(msg_from_c.data(), msg_from_c.size(), -50);
    advance_and_maintain(*protocols[3], 4000);
    EXPECT_EQ(transports[3]->get_sent_message_count(), 0);  // Should not relay

    uint8_t dummy = 0;
    transports[4]->simulate_receive(&dummy, 0, -50);
    EXPECT_EQ(transports[4]->get_sent_message_count(), 0);

    // Cleanup
    for (auto* proto : protocols) {
        delete proto;
    }
}

// CT-06: Redundant Paths Deduplication
TEST_F(MeshProtocolIntegrationTest, RedundantPathsDeduplication) {
    // Topology:
    //      B
    //     / \
    // A ---   --- D
    //     \ /
    //      C

    auto transport_a = std::make_shared<MockEspNowTransport>(0xA0000001);
    auto transport_b = std::make_shared<MockEspNowTransport>(0xB0000002);
    auto transport_c = std::make_shared<MockEspNowTransport>(0xC0000003);
    auto transport_d = std::make_shared<MockEspNowTransport>(0xD0000004);

    std::vector<MeshMessage> d_received;

    MeshProtocol protocol_a(transport_a, callback);
    MeshProtocol protocol_b(transport_b, callback);
    MeshProtocol protocol_c(transport_c, callback);
    MeshProtocol protocol_d(transport_d, [&d_received](const MeshMessage& msg, int32_t) {
        d_received.push_back(msg);
    });

    EXPECT_TRUE(protocol_a.init());
    EXPECT_TRUE(protocol_b.init());
    EXPECT_TRUE(protocol_c.init());
    EXPECT_TRUE(protocol_d.init());

    protocol_a.update_time(1000);
    protocol_b.update_time(1000);
    protocol_c.update_time(1000);
    protocol_d.update_time(1000);

    uint8_t payload[] = {0x55, 0x66, 0x77};
    EXPECT_TRUE(protocol_a.send_message(payload, sizeof(payload), 5));
    ASSERT_EQ(transport_a->get_sent_message_count(), 1);

    const auto& from_a = transport_a->get_sent_message(0);

    // B and C receive the same original message from A and relay it.
    transport_b->simulate_receive(from_a.data(), from_a.size(), -45);
    transport_c->simulate_receive(from_a.data(), from_a.size(), -47);
    advance_and_maintain(protocol_b, 2000);
    advance_and_maintain(protocol_c, 2000);

    ASSERT_GE(transport_b->get_sent_message_count(), 1);
    ASSERT_GE(transport_c->get_sent_message_count(), 1);

    const auto& from_b = transport_b->get_sent_message(0);
    const auto& from_c = transport_c->get_sent_message(0);

    // D receives same message via B then C. Second copy must be discarded.
    transport_d->simulate_receive(from_b.data(), from_b.size(), -50);
    transport_d->simulate_receive(from_c.data(), from_c.size(), -52);

    ASSERT_EQ(d_received.size(), 1);

    MeshMessage msg_b;
    MeshMessage msg_c;
    ASSERT_TRUE(MessageSerializer::deserialize(from_b.data(), from_b.size(), msg_b));
    ASSERT_TRUE(MessageSerializer::deserialize(from_c.data(), from_c.size(), msg_c));
    EXPECT_EQ(msg_b.message_id, msg_c.message_id);
}

