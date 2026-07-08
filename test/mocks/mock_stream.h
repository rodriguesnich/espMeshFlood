#ifndef ESPMESHFLOOD_TEST_MOCKS_MOCK_STREAM_H
#define ESPMESHFLOOD_TEST_MOCKS_MOCK_STREAM_H

#include <cstdint>
#include <deque>
#include <vector>

namespace espMeshFlood {
namespace test {

/**
 * @brief Minimal stream mock for testing SerialHostTransport on native targets.
 *
 * Satisfies the TStream concept expected by SerialHostTransport:
 *   void   begin(uint32_t baud)
 *   int    available()
 *   int    read()
 *   size_t write(const uint8_t* buf, size_t len)
 *   void   flush()
 *
 * Incoming bytes are loaded via push_rx().
 * Outgoing bytes are captured in tx_data() for inspection.
 */
class MockStream {
public:
    void begin(uint32_t /*baud*/) {}

    int available() {
        return static_cast<int>(rx_data_.size());
    }

    int read() {
        if (rx_data_.empty()) return -1;
        const uint8_t b = rx_data_.front();
        rx_data_.pop_front();
        return static_cast<int>(b);
    }

    size_t write(const uint8_t* buf, size_t size) {
        tx_data_.insert(tx_data_.end(), buf, buf + size);
        return size;
    }

    size_t write(uint8_t b) {
        tx_data_.push_back(b);
        return 1;
    }

    void flush() {}

    // ---- Test helpers --------------------------------------------------------

    /** Load bytes into the RX buffer to be returned by read(). */
    void push_rx(const uint8_t* data, size_t len) {
        rx_data_.insert(rx_data_.end(), data, data + len);
    }

    /** All bytes written by the transport since the last clear_tx(). */
    const std::vector<uint8_t>& tx_data() const { return tx_data_; }

    void clear_tx() { tx_data_.clear(); }

private:
    std::deque<uint8_t>  rx_data_;
    std::vector<uint8_t> tx_data_;
};

}  // namespace test
}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_TEST_MOCKS_MOCK_STREAM_H
