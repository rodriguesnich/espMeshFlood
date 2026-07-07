#ifndef ESPMESHFLOOD_HOST_SERIAL_HOST_TRANSPORT_H
#define ESPMESHFLOOD_HOST_SERIAL_HOST_TRANSPORT_H

#include "espMeshFlood/host/host_transport.h"
#include "espMeshFlood/host/frame_parser.h"

namespace espMeshFlood {

/**
 * @brief Serial host transport — implements IHostTransport over a UART stream.
 *
 * Templated on TStream so it works with both Arduino's HardwareSerial
 * (on-device) and MockStream (unit tests) without any Arduino headers.
 *
 * Usage (Arduino):
 *   SerialHostTransport<HardwareSerial> host_transport(Serial, 115200);
 *
 * Usage (tests):
 *   SerialHostTransport<MockStream> host_transport(mock_stream);
 *
 * Call poll() from loop() to drain incoming serial bytes into the frame parser.
 * Outbound frames are written synchronously inside send().
 *
 * TStream requirements:
 *   void   begin(uint32_t baud)
 *   int    available()
 *   int    read()
 *   size_t write(const uint8_t* buf, size_t len)
 *   void   flush()
 */
template <typename TStream>
class SerialHostTransport : public IHostTransport {
public:
    explicit SerialHostTransport(TStream& stream, uint32_t baud = 115200)
        : stream_(stream),
          baud_(baud),
          parser_([this](const uint8_t* data, size_t len) {
              if (receive_cb_) receive_cb_(data, len);
          }) {}

    bool init() override {
        stream_.begin(baud_);
        return true;
    }

    /**
     * @brief Wrap data in the standard frame and write it to the stream.
     *
     * Frame: [0xAA][len_lo][len_hi][protobuf bytes...]
     */
    void send(const uint8_t* data, size_t length) override {
        const uint8_t header[3] = {
            FrameParser::MAGIC_BYTE,
            static_cast<uint8_t>(length & 0xFF),
            static_cast<uint8_t>((length >> 8) & 0xFF)
        };
        stream_.write(header, sizeof(header));
        stream_.write(data, length);
        stream_.flush();
    }

    void register_receive_callback(HostReceiveCallback cb) override {
        receive_cb_ = std::move(cb);
    }

    bool is_connected() const override { return true; }

    /**
     * @brief Drain available serial bytes into the frame parser.
     *
     * Call this from loop() on every iteration.
     */
    void poll() {
        while (stream_.available()) {
            const uint8_t byte = static_cast<uint8_t>(stream_.read());
            parser_.feed(&byte, 1);
        }
    }

private:
    TStream&            stream_;
    uint32_t            baud_;
    FrameParser         parser_;
    HostReceiveCallback receive_cb_;
};

}  // namespace espMeshFlood

#endif  // ESPMESHFLOOD_HOST_SERIAL_HOST_TRANSPORT_H
