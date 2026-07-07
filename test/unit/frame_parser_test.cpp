#include <gtest/gtest.h>
#include <vector>
#include "espMeshFlood/host/frame_parser.h"

using namespace espMeshFlood;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> build_frame(const uint8_t* payload, uint16_t len) {
    std::vector<uint8_t> frame;
    frame.push_back(FrameParser::MAGIC_BYTE);
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    frame.insert(frame.end(), payload, payload + len);
    return frame;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class FrameParserTest : public ::testing::Test {
protected:
    std::vector<uint8_t> received_;
    int                  callback_count_ = 0;

    FrameParser make_parser() {
        return FrameParser([this](const uint8_t* data, size_t len) {
            ++callback_count_;
            received_.insert(received_.end(), data, data + len);
        });
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(FrameParserTest, ParsesCompleteFrame) {
    auto parser = make_parser();
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    const auto frame = build_frame(payload, sizeof(payload));
    parser.feed(frame.data(), frame.size());

    EXPECT_EQ(callback_count_, 1);
    EXPECT_EQ(received_, std::vector<uint8_t>(payload, payload + sizeof(payload)));
}

TEST_F(FrameParserTest, HandlesStreamingByteByByte) {
    auto parser = make_parser();
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const auto frame = build_frame(payload, sizeof(payload));
    for (const uint8_t b : frame) parser.feed(&b, 1);

    EXPECT_EQ(callback_count_, 1);
    EXPECT_EQ(received_, std::vector<uint8_t>(payload, payload + sizeof(payload)));
}

TEST_F(FrameParserTest, ParsesMultipleBackToBackFrames) {
    auto parser = make_parser();
    const uint8_t p1[] = {0x11};
    const uint8_t p2[] = {0x22, 0x33};
    auto frames = build_frame(p1, sizeof(p1));
    const auto f2 = build_frame(p2, sizeof(p2));
    frames.insert(frames.end(), f2.begin(), f2.end());

    parser.feed(frames.data(), frames.size());

    EXPECT_EQ(callback_count_, 2);
    ASSERT_EQ(received_.size(), 3u);
    EXPECT_EQ(received_[0], 0x11);
    EXPECT_EQ(received_[1], 0x22);
    EXPECT_EQ(received_[2], 0x33);
}

TEST_F(FrameParserTest, SkipsGarbageBeforeMagicByte) {
    auto parser = make_parser();
    const uint8_t garbage[] = {0x11, 0x22, 0x33, 0xFF};
    const uint8_t payload[] = {0x55};
    const auto frame = build_frame(payload, sizeof(payload));

    parser.feed(garbage, sizeof(garbage));
    parser.feed(frame.data(), frame.size());

    EXPECT_EQ(callback_count_, 1);
    ASSERT_EQ(received_.size(), 1u);
    EXPECT_EQ(received_[0], 0x55);
}

TEST_F(FrameParserTest, RejectsZeroLengthFrame) {
    auto parser = make_parser();
    const uint8_t zero_len[] = {FrameParser::MAGIC_BYTE, 0x00, 0x00};
    parser.feed(zero_len, sizeof(zero_len));
    EXPECT_EQ(callback_count_, 0);
}

TEST_F(FrameParserTest, RejectsExcessiveLengthFrame) {
    auto parser = make_parser();
    // Length field = 0xFFFF (65535) — larger than MAX_PAYLOAD
    const uint8_t bad[] = {FrameParser::MAGIC_BYTE, 0xFF, 0xFF};
    parser.feed(bad, sizeof(bad));
    EXPECT_EQ(callback_count_, 0);
}

TEST_F(FrameParserTest, RecoversAfterInvalidLength) {
    auto parser = make_parser();
    const uint8_t bad[] = {FrameParser::MAGIC_BYTE, 0xFF, 0xFF};
    const uint8_t payload[] = {0x42};
    const auto good = build_frame(payload, sizeof(payload));

    parser.feed(bad, sizeof(bad));       // bad frame — state resets
    parser.feed(good.data(), good.size()); // good frame — parsed correctly

    EXPECT_EQ(callback_count_, 1);
    ASSERT_EQ(received_.size(), 1u);
    EXPECT_EQ(received_[0], 0x42);
}

TEST_F(FrameParserTest, ResetClearsStateAndAllowsNewFrame) {
    auto parser = make_parser();
    // Feed a partial frame, then reset, then send a complete frame.
    const uint8_t partial[] = {FrameParser::MAGIC_BYTE, 0x05, 0x00, 0x01};
    parser.feed(partial, sizeof(partial));
    parser.reset();

    const uint8_t payload[] = {0xAB};
    const auto frame = build_frame(payload, sizeof(payload));
    parser.feed(frame.data(), frame.size());

    EXPECT_EQ(callback_count_, 1);
}
