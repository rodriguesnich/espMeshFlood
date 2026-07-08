#include "espMeshFlood/host/frame_parser.h"
#include <cstring>

namespace espMeshFlood {

FrameParser::FrameParser(FrameCallback callback)
    : callback_(std::move(callback)) {
    std::memset(rx_buffer_, 0, sizeof(rx_buffer_));
}

void FrameParser::reset() {
    state_           = State::WAIT_MAGIC;
    rx_buffer_pos_   = 0;
    expected_length_ = 0;
}

void FrameParser::feed(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        const uint8_t byte = data[i];

        switch (state_) {
            case State::WAIT_MAGIC:
                if (byte == MAGIC_BYTE) {
                    rx_buffer_pos_ = 0;
                    state_ = State::WAIT_LENGTH;
                }
                break;

            case State::WAIT_LENGTH:
                reinterpret_cast<uint8_t*>(&expected_length_)[rx_buffer_pos_++] = byte;
                if (rx_buffer_pos_ >= LENGTH_SIZE) {
                    if (expected_length_ > 0 && expected_length_ <= MAX_PAYLOAD) {
                        rx_buffer_pos_ = 0;
                        state_ = State::WAIT_PAYLOAD;
                    } else {
                        reset();
                    }
                }
                break;

            case State::WAIT_PAYLOAD:
                rx_buffer_[rx_buffer_pos_++] = byte;
                if (rx_buffer_pos_ >= expected_length_) {
                    if (callback_) {
                        callback_(rx_buffer_, expected_length_);
                    }
                    reset();
                }
                break;
        }
    }
}

}  // namespace espMeshFlood
