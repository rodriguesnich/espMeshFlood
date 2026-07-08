#include <Arduino.h>
#include <espMeshFlood/efmp.h>
#include <espMeshFlood/types.h>
#include <espMeshFlood/serialization/message_serializer.h>
#include <espMeshFlood/transport/esp_now_transport_impl.h>
#include <espMeshFlood/host/ble_host_transport.h>
#include <cstring>

using namespace espMeshFlood;

// Host transport — BLE GATT server with the unified EFMP framing protocol.
// Device name is finalised in setup() after the mesh node ID is known.
BleHostTransport host_transport;  // default name "MeshNode" until set_device_name() is called

// Global timing
unsigned long last_heartbeat_time = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 30000;
unsigned long last_maintenance_time = 0;
const unsigned long MAINTENANCE_INTERVAL_MS = 1000;

// Statistics
uint32_t messages_sent = 0;
uint32_t messages_received = 0;
uint32_t heartbeat_count = 0;

bool sendHeartbeat() {
    MeshMessage msg;
    msg.message_id  = heartbeat_count + 1;
    msg.sender_id   = EspMeshFlood::instance().get_node_id();
    msg.timestamp   = millis();
    msg.type        = MessageType::HEARTBEAT;
    msg.ttl         = 3;
    msg.relay_count = 0;

    char heartbeat_payload[32];
    snprintf(heartbeat_payload, sizeof(heartbeat_payload), "HB:%lu", millis() / 1000);
    const size_t payload_size = std::strlen(heartbeat_payload);
    msg.payload.resize(payload_size);
    std::memcpy(msg.payload.data(), heartbeat_payload, payload_size);

    uint8_t serialized[MessageSerializer::max_size()];
    const size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
    if (serialized_size == 0) return false;

    const bool sent = EspMeshFlood::instance().send_message(serialized, serialized_size, msg.ttl);
    if (sent) {
        heartbeat_count++;
        messages_sent++;
        host_transport.send(serialized, serialized_size);
    }
    return sent;
}

void on_mesh_message_received(const MeshMessage& msg, int32_t /*rssi*/) {
    messages_received++;

    // Skip our own heartbeats echoing back from the mesh — already sent in sendHeartbeat().
    if (msg.type == MessageType::HEARTBEAT &&
        msg.sender_id == EspMeshFlood::instance().get_node_id()) {
        return;
    }

    uint8_t serialized[MessageSerializer::max_size()];
    const size_t serialized_size = MessageSerializer::serialize(msg, serialized, sizeof(serialized));
    if (serialized_size > 0) {
        host_transport.send(serialized, serialized_size);
    }
}

void setup() {
    // Serial is used for debug output only — no framed protocol over serial.
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 Mesh Node (BLE) starting...");

    // Initialise mesh first so we can read the node ID for the BLE device name.
    auto transport = std::make_shared<espMeshFlood::EspNowTransportImpl>();
    if (!EspMeshFlood::instance().init(on_mesh_message_received, transport)) {
        Serial.println("Failed to initialize mesh!");
        while (1) delay(1000);
    }

    const uint32_t node_id = EspMeshFlood::instance().get_node_id();
    Serial.print("Node ID: ");
    Serial.println(node_id);

    // Derive a human-readable BLE device name from the node ID.
    char ble_name[20];
    snprintf(ble_name, sizeof(ble_name), "MeshNode-%04X", node_id & 0xFFFF);
    host_transport.set_device_name(ble_name);

    // Register host → mesh receive callback.
    // BleHostTransport calls this from its GATT write handler (NimBLE task context).
    // FrameParser strips framing; callback receives raw protobuf bytes.
    host_transport.register_receive_callback([](const uint8_t* data, size_t len) {
        MeshMessage msg;
        if (MessageSerializer::deserialize(data, len, msg)) {
            EspMeshFlood::instance().send_message(data, len, msg.ttl);
            messages_sent++;
        }
    });

    // Start BLE advertising — clients can now connect via Web Bluetooth.
    host_transport.init();

    Serial.print("BLE advertising as: ");
    Serial.println(ble_name);
    Serial.println("Service UUID: 4FAFC201-1FB5-459E-8FCC-C5C9C331914B");
    Serial.println("TX char:      BEB5483E-36E1-4688-B7F5-EA07361B26A8  (subscribe for notifications)");
    Serial.println("RX char:      BEB5483E-36E1-4688-B7F5-EA07361B26A9  (write to send messages)");
}

void loop() {
    const unsigned long now = millis();

    // BLE receive is handled in NimBLE's task context — no poll() needed.

    if (now - last_heartbeat_time >= HEARTBEAT_INTERVAL_MS) {
        last_heartbeat_time = now;
        sendHeartbeat();
    }

    if (now - last_maintenance_time >= MAINTENANCE_INTERVAL_MS) {
        last_maintenance_time = now;
        EspMeshFlood::instance().update_time(now);
        EspMeshFlood::instance().do_maintenance();
    }

    delay(100);
}