// BLE support is only available in BLE firmware builds on-device.
#if defined(HOST_TRANSPORT_BLE) && !defined(UNIT_TESTING)

#include "espMeshFlood/host/ble_host_transport.h"
#include "espMeshFlood/serialization/message_serializer.h"
#include <cstring>

namespace espMeshFlood {

BleHostTransport::BleHostTransport(const char* device_name)
    : parser_([this](const uint8_t* data, size_t len) {
          if (receive_cb_) receive_cb_(data, len);
      }) {
    set_device_name(device_name);
}

void BleHostTransport::set_device_name(const char* name) {
    std::strncpy(device_name_, name, DEVICE_NAME_MAX - 1);
    device_name_[DEVICE_NAME_MAX - 1] = '\0';
}

bool BleHostTransport::init() {
    NimBLEDevice::init(device_name_);
    NimBLEDevice::setMTU(512);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(this);

    NimBLEService* service = server->createService(SERVICE_UUID);

    tx_char_ = service->createCharacteristic(TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    NimBLECharacteristic* rx_char = service->createCharacteristic(
        RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    rx_char->setCallbacks(this);

    service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    return true;
}

void BleHostTransport::send(const uint8_t* data, size_t length) {
    if (!tx_char_ || !is_connected()) return;

    // Build framed buffer: [0xAA][len_lo][len_hi][protobuf...]
    uint8_t frame[MessageSerializer::max_size() + 3];
    frame[0] = FrameParser::MAGIC_BYTE;
    frame[1] = static_cast<uint8_t>(length & 0xFF);
    frame[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
    std::memcpy(frame + 3, data, length);

    tx_char_->notify(frame, length + 3);
}

void BleHostTransport::register_receive_callback(HostReceiveCallback cb) {
    receive_cb_ = std::move(cb);
}

bool BleHostTransport::is_connected() const {
    NimBLEServer* server = NimBLEDevice::getServer();
    return server && server->getConnectedCount() > 0;
}

void BleHostTransport::onConnect(NimBLEServer* /*server*/) {}

void BleHostTransport::onDisconnect(NimBLEServer* server) {
    // Restart advertising so another client can connect after disconnect.
    server->startAdvertising();
}

void BleHostTransport::onWrite(NimBLECharacteristic* characteristic) {
    NimBLEAttValue val = characteristic->getValue();
    if (val.length() > 0) {
        parser_.feed(val.data(), val.length());
    }
}

}  // namespace espMeshFlood

#endif  // defined(HOST_TRANSPORT_BLE) && !defined(UNIT_TESTING)
