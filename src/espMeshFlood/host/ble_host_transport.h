#ifndef ESPMESHFLOOD_HOST_BLE_HOST_TRANSPORT_H
#define ESPMESHFLOOD_HOST_BLE_HOST_TRANSPORT_H

#include "espMeshFlood/host/host_transport.h"
#include "espMeshFlood/host/frame_parser.h"

// BLE support is only available in BLE firmware builds on-device.
#if defined(HOST_TRANSPORT_BLE) && !defined(UNIT_TESTING)

#include <NimBLEDevice.h>

namespace espMeshFlood {

/**
 * @brief BLE GATT host transport — implements IHostTransport over BLE (NimBLE-Arduino).
 *
 * Exposes a custom GATT service compatible with the Web Bluetooth API:
 *
 *   Service:  4FAFC201-1FB5-459E-8FCC-C5C9C331914B
 *   TX char:  BEB5483E-36E1-4688-B7F5-EA07361B26A8  (Notify  — ESP → client)
 *   RX char:  BEB5483E-36E1-4688-B7F5-EA07361B26A9  (Write NR — client → ESP)
 *
 * Wire framing is identical to the serial protocol:
 *   [0xAA magic: 1 byte][length: 2 bytes LE][protobuf payload: N bytes]
 *
 * Typical setup() usage:
 *   BleHostTransport host_transport;           // default name "MeshNode"
 *   // ... init mesh first to obtain node_id ...
 *   char name[20];
 *   snprintf(name, sizeof(name), "MeshNode-%04X", node_id & 0xFFFF);
 *   host_transport.set_device_name(name);
 *   host_transport.register_receive_callback(...);
 *   host_transport.init();                     // starts BLE advertising
 *
 * Inbound frames are parsed from GATT write callbacks (interrupt-safe through
 * NimBLE's task context). No poll() is needed.
 *
 * MTU: requests 512 bytes on init; typically negotiates to 247 bytes on
 * Android/Chrome (244 byte usable payload — sufficient for the max 233-byte frame).
 */
class BleHostTransport : public IHostTransport,
                         public NimBLEServerCallbacks,
                         public NimBLECharacteristicCallbacks {
public:
    static constexpr const char* SERVICE_UUID = "4FAFC201-1FB5-459E-8FCC-C5C9C331914B";
    static constexpr const char* TX_CHAR_UUID = "BEB5483E-36E1-4688-B7F5-EA07361B26A8";
    static constexpr const char* RX_CHAR_UUID = "BEB5483E-36E1-4688-B7F5-EA07361B26A9";

    explicit BleHostTransport(const char* device_name = "MeshNode");

    /**
     * @brief Update the BLE device name. Must be called before init().
     */
    void set_device_name(const char* name);

    // IHostTransport
    bool init() override;
    void send(const uint8_t* data, size_t length) override;
    void register_receive_callback(HostReceiveCallback cb) override;
    bool is_connected() const override;

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer* server) override;
    void onDisconnect(NimBLEServer* server) override;

    // NimBLECharacteristicCallbacks
    void onWrite(NimBLECharacteristic* characteristic) override;

private:
    static constexpr size_t DEVICE_NAME_MAX = 20;

    char                   device_name_[DEVICE_NAME_MAX];
    NimBLECharacteristic*  tx_char_ = nullptr;
    FrameParser            parser_;
    HostReceiveCallback    receive_cb_;
};

}  // namespace espMeshFlood

#endif  // defined(HOST_TRANSPORT_BLE) && !defined(UNIT_TESTING)
#endif  // ESPMESHFLOOD_HOST_BLE_HOST_TRANSPORT_H
