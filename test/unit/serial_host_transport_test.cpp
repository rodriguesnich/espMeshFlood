#include <gtest/gtest.h>
#include <vector>
#include "espMeshFlood/host/serial_host_transport.h"
#include "mock_stream.h"

using namespace espMeshFlood;
using namespace espMeshFlood::test;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> make_frame(const uint8_t* payload, uint16_t len) {
    std::vector<uint8_t> frame;
    frame.push_back(FrameParser::MAGIC_BYTE);
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    frame.insert(frame.end(), payload, payload + len);
    return frame;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class SerialTransportTest : public ::testing::Test {
protected:
    MockStream stream;
    SerialHostTransport<MockStream> transport{stream, 115200};
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(SerialTransportTest, InitSucceeds) {
    EXPECT_TRUE(transport.init());
}

TEST_F(SerialTransportTest, IsAlwaysConnected) {
    EXPECT_TRUE(transport.is_connected());
}

TEST_F(SerialTransportTest, SendWritesCorrectFrame) {
    transport.init();
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE};
    transport.send(payload, sizeof(payload));

    const auto& tx = stream.tx_data();
    ASSERT_EQ(tx.size(), 6u);
    EXPECT_EQ(tx[0], FrameParser::MAGIC_BYTE);  // magic
    EXPECT_EQ(tx[1], 0x03);                     // length lo
    EXPECT_EQ(tx[2], 0x00);                     // length hi
    EXPECT_EQ(tx[3], 0xDE);
    EXPECT_EQ(tx[4], 0xAD);
    EXPECT_EQ(tx[5], 0xBE);
}

TEST_F(SerialTransportTest, SendLittleEndianLength) {
    transport.init();
    // Payload of 256 bytes to verify multi-byte length encoding
    std::vector<uint8_t> big_payload(256, 0xAA);
    transport.send(big_payload.data(), big_payload.size());

    const auto& tx = stream.tx_data();
    ASSERT_GE(tx.size(), 3u);
    EXPECT_EQ(tx[1], 0x00);  // 256 & 0xFF = 0
    EXPECT_EQ(tx[2], 0x01);  // 256 >> 8  = 1
}

TEST_F(SerialTransportTest, PollFiresReceiveCallbackOnCompleteFrame) {
    transport.init();

    std::vector<uint8_t> received;
    transport.register_receive_callback([&](const uint8_t* d, size_t l) {
        received.assign(d, d + l);
    });

    const uint8_t payload[] = {0x01, 0x02, 0x03};
    const auto frame = make_frame(payload, sizeof(payload));
    stream.push_rx(frame.data(), frame.size());
    transport.poll();

    ASSERT_EQ(received.size(), 3u);
    EXPECT_EQ(received[0], 0x01);
    EXPECT_EQ(received[1], 0x02);
    EXPECT_EQ(received[2], 0x03);
}

TEST_F(SerialTransportTest, PollHandlesStreamingBytes) {
    transport.init();

    int callback_count = 0;
    transport.register_receive_callback([&](const uint8_t*, size_t) {
        ++callback_count;
    });

    const uint8_t payload[] = {0x55};
    const auto frame = make_frame(payload, sizeof(payload));

    // Feed one byte at a time by pushing individually and polling
    for (const uint8_t b : frame) {
        stream.push_rx(&b, 1);
        transport.poll();
    }

    EXPECT_EQ(callback_count, 1);
}

TEST_F(SerialTransportTest, PollIgnoresGarbageBeforeMagic) {
    transport.init();

    int callback_count = 0;
    transport.register_receive_callback([&](const uint8_t*, size_t) {
        ++callback_count;
    });

    const uint8_t garbage[] = {0x00, 0xFF, 0x12};
    const uint8_t payload[] = {0x77};
    const auto frame = make_frame(payload, sizeof(payload));

    stream.push_rx(garbage, sizeof(garbage));
    stream.push_rx(frame.data(), frame.size());
    transport.poll();

    EXPECT_EQ(callback_count, 1);
}
